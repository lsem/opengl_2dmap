#include <common/global.h>
#include <tuple>
#include <vector>

#pragma once
namespace map_compiler {

// see shpdump.c
using part_points_t = std::vector<gg::p32>;
using shape_parts_t = std::vector<part_points_t>;
using shapes_t = std::vector<shape_parts_t>;

shapes_t load_shapes(const fs::path &shape_file_path);

} // namespace map_compiler