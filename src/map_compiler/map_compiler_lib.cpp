
#include <algorithm>
#include <common/global.h>
#include <common/log.h>
#include <cstdlib>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gg/gg.h>
#include <list>
#include <shapefil.h> // shapelib

#include "map_compiler_lib.h"

namespace fs = std::filesystem;
namespace map_compiler {

shapes_t load_shapes(const fs::path &shape_file_path) {
  if (!fs::exists(shape_file_path)) {
    throw std::runtime_error(
        fmt::format("Path {} does not exist", shape_file_path));
  }

  SHPHandle lands_shp = SHPOpen(shape_file_path.c_str(), "rb");
  if (!lands_shp) {
    throw std::runtime_error(fmt::format(
        "failed opening lands dbf. error: {}({})", strerror(errno), errno));
  }

  double adfMinBound[4], adfMaxBound[4];
  int nShapeType, nEntities;
  // https://www.esri.com/content/dam/esrisites/sitecore-archive/Files/Pdfs/library/whitepapers/pdfs/shapefile.pdf
  SHPGetInfo(lands_shp, &nEntities, &nShapeType, adfMinBound, adfMaxBound);
  if (nShapeType != SHPT_POLYGON) {
    // see
    // https://engineering.purdue.edu/~what/romin/WEBGIS/shapelib/shapelib-1.2.9/shp_api.html

    // https://stackoverflow.com/questions/1165647/how-to-determine-if-a-list-of-polygon-points-are-in-clockwise-order
    throw std::runtime_error(
        fmt::format("Unexpected type of shape: {}", SHPTypeName(nShapeType)));
  }

  shapes_t shapes;
  //   fmt::print("Shapefile Type: {}   # of Shapes: {}\n\n",
  //              SHPTypeName(nShapeType), nEntities);
  for (int i = 0; i < nEntities; i++) {
    // log_debug("reading entity {}", i);
    auto *shape = SHPReadObject(lands_shp, i);
    if (!shape) {
      throw std::runtime_error(
          fmt::format("failed reading shape object {}", i));
    }

    // log_debug("Read shape; vertices count: {}, parts count: {}",
    //           shape->nVertices, shape->nParts);
    shape_parts_t shape_parts;
    for (auto part_i = 0; part_i < shape->nParts; ++part_i) {
      // log_debug("part name: {}",
      // SHPPartTypeName(shape->panPartType[part_i]));
      assert(shape->panPartType[part_i] == SHPP_RING);

      // part is individual polygon?

      const size_t part_start_offset = shape->panPartStart[part_i];
      const size_t part_end_offset = part_i == shape->nParts - 1
                                         ? shape->nVertices
                                         : shape->panPartStart[part_i + 1];

      assert(shape->padfX[part_start_offset] ==
             shape->padfX[part_end_offset - 1]);

      part_points_t part_points;
      // read part
      // also detect orientation of RING. Not sure if this correct to calculate
      // in sperical space using method from Euclidian space.
      for (size_t vi = part_start_offset; vi != part_end_offset; ++vi) {
        auto x = shape->padfX[vi];
        auto y = shape->padfY[vi];
        x = gg::mercator::project_lon(gg::mercator::clamp_lon_to_valid(x));
        y = gg::mercator::project_lat(gg::mercator::clamp_lat_to_valid(y));
        gg::gpt_units_t ux = gg::lon_to_x(x);
        gg::gpt_units_t uy = gg::lat_to_y(y);
        part_points.emplace_back(ux, uy);
      }
      part_points =
          gg::utils::eliminate_parallel_segments(std::move(part_points));
      if (part_points.size() < 3) {
        log_warn("part {} of shape {} is too simple, remove from output", i,
                 part_i);
      } else {
        shape_parts.emplace_back(std::move(part_points));
      }
    }

    shapes.emplace_back(std::move(shape_parts));

    SHPDestroyObject(shape);
  }

  log_debug("Loaded {} shapes:", shapes.size());
  for (auto &shape : shapes) {
    size_t part_size = 0;
    for (auto &part : shape) {
      part_size += part.size();
    }
    log_debug("  {} parts ({} points) ", shape.size(), part_size);
  }

  return std::move(shapes);
}

} // namespace map_compiler
