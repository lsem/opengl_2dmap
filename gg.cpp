#include "gg.h"
#include <limits>
#include <type_traits>

namespace gg {

namespace {
// todo: use inline func
#define DEG2RAD(a) ((a) / (180 / M_PI))
#define RAD2DEG(a) ((a) * (180 / M_PI))
#define EARTH_RADIUS 6378137

/* The following functions take their parameter and return their result in
 * degrees */

double y2lat_d(double y) {
  return RAD2DEG(atan(exp(DEG2RAD(y))) * 2 - M_PI / 2);
}
double x2lon_d(double x) {
  return x;
}

double lat2y_d(double lat) {
  return RAD2DEG(log(tan(DEG2RAD(lat) / 2 + M_PI / 4)));
}

double lon2x_d(double lon) {
  return lon;
}

/* The following functions take their parameter in something close to meters,
 * along the equator, and return their result in degrees */

double y2lat_m(double y) {
  return RAD2DEG(2 * atan(exp(y / EARTH_RADIUS)) - M_PI / 2);
}
double x2lon_m(double x) {
  return RAD2DEG(x / EARTH_RADIUS);
}

/* The following functions take their parameter in degrees, and return their
 * result in something close to meters, along the equator */

double lat2y_m(double lat) {
  return log(tan(DEG2RAD(lat) / 2 + M_PI / 4)) * EARTH_RADIUS;
}
double lon2x_m(double lon) {
  return DEG2RAD(lon) * EARTH_RADIUS;
}

}  // namespace

#define EXTERN

EXTERN punits_t lat_to_y_pu(double lat) {
  static_assert(std::is_same_v<punits_t, uint32_t>);
  return (lat * std::numeric_limits<uint32_t>::max()) / 360.0;
}

EXTERN double y_pu_to_lat(punits_t y) {
  return 0.0;
}

}  // namespace gg