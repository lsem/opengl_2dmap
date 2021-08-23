#include "gg/gg.h"
#include <limits>
#include <type_traits>

namespace gg {

#define EXTERN

constexpr auto U32_MAX = std::numeric_limits<uint32_t>::max();

// new methods.
EXTERN gpt_units_t lat_to_y(double lat) {
  return U32_MAX / 360.0 * (lat + 90.0);
}
EXTERN gpt_units_t lon_to_x(double lon) {
  return (U32_MAX / 360.0) * (lon + 180.0);
}
EXTERN double y_to_lat(gpt_units_t y) {
  return (360.0 / U32_MAX) * y - 90.0;
}
EXTERN double x_to_lon(gpt_units_t x) {
  return (360.0 / U32_MAX) * x - 180.0;
}
EXTERN gpt_t wgs_to_gpt(double lat, double lon) {
    return { lon_to_x(lon), lat_to_y(lat) };
}
EXTERN wgs84_t to_wgs(const gpt_t& pt) {
    return { y_to_lat(pt.y), x_to_lon(pt.x) };
}
tile_id_t tile_id_by_pt(gpt_t pt, uint32_t level) {
    assert(level <= 15);
    auto x = pt.x >> (31 - level);
    auto y = pt.y >> (31 - level);
    return { static_cast<uint16_t>(x), static_cast<uint16_t>(y) };
}
gbb_t tile_bb(tile_at_level_t tile) {
    gbb_t box;
    box.top_left = { (uint32_t)tile.id.x << (31 - tile.level), (uint32_t)tile.id.y << (31 - tile.level) };
    box.width = box.height = 1 << (31 - tile.level);
    return box;
}
tile_at_level_t root_tile() { return { 0, 0 }; }
tile_at_level_t parent_tile(tile_at_level_t tile) {
    assert(tile.level > 0);
    tile.id.x /= 2;
    tile.id.y /= 2;
    tile.level -= 1;
    return tile;
}
std::array<tile_id_t, 4> children_tiles(tile_at_level_t tile) {
    std::array<tile_id_t, 4> res = {
        tile_id_t{tile.id.x * 2 + 0, tile.id.y * 2 + 0},
        tile_id_t{tile.id.x * 2 + 1, tile.id.y * 2 + 0},
        tile_id_t{tile.id.x * 2 + 0, tile.id.y * 2 + 1},
        tile_id_t{tile.id.x * 2 + 1, tile.id.y * 2 + 1} };
    return res;
}

}  // namespace gg