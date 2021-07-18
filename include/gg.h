#pragma once

#include <math.h>
#include <cstdint>

// general geometry
namespace gg {
using punits_t = uint32_t;
punits_t lat_to_y_pu(double lat);
double y_pu_to_lat(punits_t x);
}  // namespace gg
