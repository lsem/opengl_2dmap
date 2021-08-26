#include "gg/gg.h"
#include <limits>
#include <type_traits>

namespace gg {

#define EXTERN

EXTERN gpt_units_t lat_to_y(double mercator_lat) {
  return U32_MAX / 360.0 * (mercator_lat + 180.0);
}
EXTERN gpt_units_t lon_to_x(double mercator_lon) {
  return (U32_MAX / 360.0) * (mercator_lon + 180.0);
}
EXTERN double y_to_lat(gpt_units_t y) { return (360.0 / U32_MAX) * y - 180.0; }
EXTERN double x_to_lon(gpt_units_t x) { return (360.0 / U32_MAX) * x - 180.0; }
EXTERN gpt_t wgs_to_gpt(double lat, double lon) {
  return {lon_to_x(lon), lat_to_y(lat)};
}
EXTERN wgs84_t to_wgs(const gpt_t &pt) {
  return {y_to_lat(pt.y), x_to_lon(pt.x)};
}
tile_id_t tile_id_by_pt(gpt_t pt, uint32_t level) {
  assert(level <= 15);
  auto x = pt.x >> (31 - level);
  auto y = pt.y >> (31 - level);
  return {static_cast<uint16_t>(x), static_cast<uint16_t>(y)};
}
gbb_t tile_bb(tile_at_level_t tile) {
  gbb_t box;
  box.top_left = {(uint32_t)tile.id.x << (31 - tile.level),
                  (uint32_t)tile.id.y << (31 - tile.level)};
  box.width = box.height = 1 << (31 - tile.level);
  return box;
}
tile_at_level_t root_tile() { return {0, 0}; }
tile_at_level_t parent_tile(tile_at_level_t tile) {
  assert(tile.level > 0);
  tile.id.x /= 2;
  tile.id.y /= 2;
  tile.level -= 1;
  return tile;
}
std::array<tile_id_t, 4> children_tiles(tile_at_level_t tile) {
  std::array<tile_id_t, 4> res = {
      tile_id_t{static_cast<uint16_t>(tile.id.x * 2 + 0),
                static_cast<uint16_t>(tile.id.y * 2 + 0)},
      tile_id_t{static_cast<uint16_t>(tile.id.x * 2 + 1),
                static_cast<uint16_t>(tile.id.y * 2 + 0)},
      tile_id_t{static_cast<uint16_t>(tile.id.x * 2 + 0),
                static_cast<uint16_t>(tile.id.y * 2 + 1)},
      tile_id_t{static_cast<uint16_t>(tile.id.x * 2 + 1),
                static_cast<uint16_t>(tile.id.y * 2 + 1)}};
  return res;
}

namespace utils {
std::vector<p32> eliminate_parallel_segments(std::vector<p32> points) {
  const size_t N = points.size();
  if (N < 3) {
    throw std::runtime_error("eliminate_parallel_segments: N < 3");
  }
  size_t l = 0;
  size_t r = 1;
  while (r < points.size() - 1) {
    gg::v2 p0(points[l]);
    gg::v2 p1(points[r]);
    gg::v2 p2(points[r + 1]);
    gg::v2 a(p0, p1);
    gg::v2 b(p1, p2);
    gg::v2 br(-b.y, b.x);
    if (std::abs(dot(a, br)) < 1e-5) {
      points[r].x =
          gg::U32_MAX; // todo: this is not good since we steal valid point
                       // which can naturally occur during tiles cuts or clamps.
      points[r].y = gg::U32_MAX;
      r++;
    } else {
      r++;
      while ((points[++l].x == gg::U32_MAX) && (points[++l].y == gg::U32_MAX)) {
        // hop through wholes
      }
    }
  }
  const size_t points_before = points.size();
  points.erase(std::remove_if(std::begin(points), std::end(points),
                              [](const gg::p32 &v) {
                                return v.x == gg::U32_MAX && v.y == gg::U32_MAX;
                              }),
               std::end(points));
  return std::move(points);
}
} // namespace utils

} // namespace gg