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
EXTERN gpt_units_t lon_to_x(double lat) {
  return (U32_MAX / 360.0) * (lat + 180.0);
}
EXTERN double y_to_lat(gpt_units_t y) {
  return (360.0 / U32_MAX) * y - 90.0;
}
EXTERN double x_to_lon(gpt_units_t x) {
  return (360.0 / U32_MAX) * x - 180.0;
}

}  // namespace gg