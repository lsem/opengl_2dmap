
#include <common/global.h>
#include <common/log.h>
#include <cstdlib>
#include <filesystem>
#include <fmt/format.h>
#include <gg/gg.h>
#include <list>
#include <shapefil.h> // shapelib

namespace fs = std::filesystem;

// todo: use snappy or similar when it comes to packing.

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


shapes_t load_shapes(const fs::path& shape_file_path) {
  if (!fs::exists(shape_file_path)) {
    throw std::runtime_error(fmt::format("Path {} does not exist", shape_file_path));
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

      //   log_debug("part: {}..{}", part_start_offset,
      //             part_start_offset + part_size);

    assert(shape->padfX[part_start_offset] == shape->padfX[part_end_offset - 1]);

      part_points_t part_points;
      // read part
      // also detect orientation of RING. Not sure if this correct to calculate
      // in sperical space using method from Euclidian space.
      double signed_area2 = 0.0;
      double x1 = NAN, y1 = NAN;
      for (size_t vi = part_start_offset; vi != part_end_offset; ++vi) {
        auto x = shape->padfX[vi];
        auto y = shape->padfY[vi];

        auto py = gg::mercator::project_lat(y);
        auto px = gg::mercator::project_lon(x);

        // log_debug("point: {},{}", x, y);
        part_points.emplace_back(x, y);
        double x2 = px;
        double y2 = py;
        if (!std::isnan(x1)) {
          assert(!std::isnan(y1));
          signed_area2 += (x1 * y2 - x2 * y1);
        }
        x1 = x2;
        y1 = y2;
      }
      if (signed_area2 < 0) {
        // clockwise
        // log_debug("Clockwise");
      } else {
        // counterclockwise
        log_warn("Counterclockwise: {}", part_i);
        continue;
      }
      shape_parts.emplace_back(std::move(part_points));
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

// int main() {
//   try {
//     log_debug("compiling lands..");
//     compile_lands();
//     log_debug("compiling lands.. DONE");

//   } catch (const std::exception &e) {
//     log_err("error: {}", e.what());
//   }
// }
