#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <ostream>

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

// Tiles stuff
// What do I know about tile id?
// I know that at each level I have division by four.
// four takes two bits: 00, 01, 10, 11
// so I need two bits to select tile at each level.
// if I want to have 16 levels then I need 32 bits to encode that.
using tile_id_t = uint32_t;
tile_id_t tile_id_by_pt(gpt_t pt);
unsigned tile_level(tile_id_t tid);
gbb_t tile_bb(tile_id_t tid);

// trying out this alias.
using p32 = gpt_t;

// todo: does it make sense to have integer vectors?
struct v2 {
  double v[2];

  v2() {} // uninitialized
  v2(double x, double y) { v[0] = x, v[1] = y; }
  v2(v2 a, v2 b) : v2(b[0] - a[0], b[1] - a[1]) {}
  v2(p32 p): v2(p.x, p.y) {}

  double x() const { return v[0]; }
  double y() const { return v[1]; }
  double operator[](size_t idx) const {
    assert(idx < 3);
    return v[idx];
  }
};

inline  v2 operator*(v2 v, double s) { return v2{v[0] * s, v[1] * s}; }
inline  v2 operator/(v2 v, double s) { return v2{v[0] / s, v[1] / s}; }
inline v2 operator+(v2 a, v2 b) { return v2{a[0] + b[0], a[1] + b[1]}; }
inline v2 operator-(v2 a, v2 b) { return v2{a[0] - b[0], a[1] - b[1]}; }
inline double dot(v2 a, v2 b) { return a[0] * b[0] + a[1] * b[1]; }
inline double len2(v2 v) { return dot(v, v); }
inline double len(v2 v) { return std::sqrt(len2(v)); }
inline v2 normalized(v2 v) { return v / len(v); }


inline std::ostream& operator<<(std::ostream& os, p32 p) {
	os << p.x << ", " << p.y;
	return os;
}

inline std::ostream& operator<<(std::ostream& os, v2 v) {
	os << v[0] << ", " << v[1];
	return os;
}


}  // namespace gg
