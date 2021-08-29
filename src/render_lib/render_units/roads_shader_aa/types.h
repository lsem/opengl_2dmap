#pragma once

#include "gg/gg.h"
#include <array>

namespace roads_shader_aa {
struct AAVertex {
    gg::p32 coords;
    uint8_t is_outer;
    // std::array<uint8_t,3> color;
    std::array<float, 3> color;
    std::array<float, 2> extent_vec;
};
} // namespace roads_shader_aa
