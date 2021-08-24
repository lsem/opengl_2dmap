#include <common/global.h>
#include <tuple>
#include <vector>

#pragma once 
namespace map_compiler {

// hierarch is the follwing:
//    one file has multiple shapes
//    each shape has multiple parts
//    each part has multiple vertices.

// see shpdump.c
using vertice_t = std::tuple<double, double>;
using part_points_t = std::vector<vertice_t>;
using shape_parts_t = std::vector<part_points_t>;
using shapes_t = std::vector<shape_parts_t>;

shapes_t load_shapes(const fs::path& shape_file_path);

} // namespace map_compiler