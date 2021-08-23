#pragma once

#define _USE_MATH_DEFINES

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <array>

// general geometry
namespace gg {

using gpt_units_t = uint32_t;
struct gpt_t {
  gpt_units_t x, y;
};
gpt_units_t lat_to_y(double lat);
gpt_units_t lon_to_x(double lon);
double y_to_lat(gpt_units_t y);
double x_to_lon(gpt_units_t x);

// geographic bounding box
struct gbb_t {
  gpt_t top_left;
  gpt_units_t width;
  gpt_units_t height;
};

struct wgs84_t {
    double lat;
    double lon;
};

gpt_t wgs_to_gpt(double lat, double lon); 
wgs84_t to_wgs(const gpt_t& pt);

// Tiles stuff
// What do I know about tile id?
// I know that at each level I have division by four.
// four takes two bits: 00, 01, 10, 11
// so I need two bits to select tile at each level.
// if I want to have 16 levels then I need 32 bits to encode that.
struct tile_id_t {
    tile_id_t() : id(0) {}
    tile_id_t(uint32_t id) : id(id) {}
    tile_id_t(uint16_t x, uint16_t y) : x(x), y(y) {}
    union {
        uint32_t id;
        struct {
            uint16_t x;
            uint16_t y;
        };
    };
};
static_assert(sizeof(tile_id_t) == sizeof(uint32_t), "32 bit");

// We don't want to play with rticky level encoding
// just store level information next to tile_id
// With mercator projecftion we are going to use entire range of 32 bits on a level 15
// so we has no waste bits to store level information
struct tile_at_level_t {
    tile_id_t id;
    uint32_t level; // 0 .. 15
};

tile_id_t tile_id_by_pt(gpt_t pt, uint32_t level);
gbb_t tile_bb(tile_at_level_t tile);

tile_at_level_t root_tile() { return { 0, 0 }; }
tile_at_level_t parent_tile(tile_at_level_t);
std::array<tile_id_t, 4> children_tiles(tile_at_level_t);

// trying out this alias.
using p32 = gpt_t;

// todo: does it make sense to have integer vectors?
struct v2 {
  double x, y;

  v2() {} // uninitialized
  v2(double x, double y) : x(x), y(y) {}
  v2(v2 a, v2 b) : v2(b[0] - a[0], b[1] - a[1]) {}
  v2(p32 p) : v2(p.x, p.y) {}

  double operator[](size_t idx) const {
    assert(idx < 3);
    if (idx == 0) {
      return x;
    } else if (idx == 1) {
      return y;
    } else {
      return {};
    }
  }
};

inline v2 operator*(v2 v, double s) { return v2{v[0] * s, v[1] * s}; }
inline v2 &operator*=(v2 &v, double s) {
  v = v * s;
  return v;
}
inline v2 operator*(double s, v2 v) { return v * s; }
inline v2 operator/(v2 v, double s) { return v2{v[0] / s, v[1] / s}; }
inline v2 operator+(v2 a, v2 b) { return v2{a[0] + b[0], a[1] + b[1]}; }
inline v2 operator-(v2 a, v2 b) { return v2{a[0] - b[0], a[1] - b[1]}; }
inline double dot(v2 a, v2 b) { return a[0] * b[0] + a[1] * b[1]; }
inline double len2(v2 v) { return dot(v, v); }
inline double len(v2 v) { return std::sqrt(len2(v)); }
inline v2 normalized(v2 v) { return v / len(v); }
inline v2 operator-(v2 v) { return v * -1; }

// this is so called scalar cross product which is solving determinant 2x2
// of matrix [v0, v1
//            u0. u1 ]
inline double cross2d(v2 v, v2 u) { return v[0] * u[1] - v[1] * u[0]; }

inline bool lines_intersection_impl(double p0_x, double p0_y, double p1_x,
                                    double p1_y, double p2_x, double p2_y,
                                    double p3_x, double p3_y, double *i_x,
                                    double *i_y) {
  // Solve system of linear equations a1x + b1y + c1 and a2x + b2y + c1
  // using cramer's rule.

  double s1_x = p1_x - p0_x;
  double s1_y = p1_y - p0_y;
  double s2_x = p3_x - p2_x;
  double s2_y = p3_y - p2_y;

  double det = s1_x * s2_y - s1_y * s2_x;
  if (std::fabs(det) < 0.00001) { // todo: use EPS instead of hardcoded
    return false;
  }

  double s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / det;
  double t = (s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / det;

  // check for segment intersection: s >= 0 && s <= 1 && t >= 0 && t <= 1
  if (i_x != NULL)
    *i_x = p0_x + (t * s1_x);
  if (i_y != NULL)
    *i_y = p0_y + (t * s1_y);
  return true;
}

inline bool lines_intersection(v2 a, v2 b, v2 p, v2 q, v2 &out_r) {
  double i_x, i_y;
  if (lines_intersection_impl(a[0], a[1], b[0], b[1], p[0], p[1], q[0], q[1],
                              &i_x, &i_y)) {
    out_r = v2{i_x, i_y};
    return true;
  } else {
    return false;
  }
}

inline std::ostream &operator<<(std::ostream &os, p32 p) {
  os << p.x << ", " << p.y;
  return os;
}

inline std::ostream &operator<<(std::ostream &os, v2 v) {
  os << v[0] << ", " << v[1];
  return os;
}

inline double deg_to_rad(double x) { return x * M_PI / 180.0; }
inline double rad_to_deg(double x) { return x * 180.0 / M_PI; }

// Mercator projection formulas.
namespace mercator {
// These functions are commented until they are needed and they have tests.
// inline double y_to_lat(double y) {
//   return rad_to_deg(atan(exp(deg_to_rad(y))) * 2 - M_PI / 2);
// }
// Projects rad, input and output inr radians.
// inline double x_to_lon(double x) { // todo: rename to unproject_lon
//   return x;
// }

inline double lat_to_y_r(double lat_r) {
  return log(tan(lat_r / 2 + M_PI / 4));
}
inline double lon_to_x_r(double lon) { return lon; }

inline double project_lat(double lat) {
  return rad_to_deg(log(tan(deg_to_rad(lat) / 2 + M_PI / 4)));
}
inline double project_lon(double lon) { return lon; }

inline gpt_units_t lat_to_yu(double lat) {
  // after projection range becomes -180..+180 assuming we clamped value to
  // -85..+85.
  // todo: should we use round() or we can just truncate is fine?
  return static_cast<uint32_t>(
      ((lat_to_y_r(deg_to_rad(lat)) + M_PI) / (2 * M_PI)) *
      std::numeric_limits<uint32_t>::max());
}
inline gpt_units_t lon_to_xu(double lon) {
  return static_cast<uint32_t>(((lon + 180.0) / 360.0) *
                               std::numeric_limits<uint32_t>::max());
}
} // namespace mercator

} // namespace gg
