#include <common/global.h>
#include <common/log.h>
#include <cstdlib>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <list>
#include <shapefil.h> // shapelib

#include "map_compiler_lib.h"

namespace fs = std::filesystem;
namespace map_compiler {

// https://www.esri.com/content/dam/esrisites/sitecore-archive/Files/Pdfs/library/whitepapers/pdfs/shapefile.pdf
shapes_t load_shapes(const fs::path &shape_file_path) {
    if (!fs::exists(shape_file_path)) {
        throw std::runtime_error(fmt::format("Path {} does not exist", shape_file_path));
    }

    // Note, on MSVC fs::path::string_type is wstring
    SHPHandle lands_shp = SHPOpen(shape_file_path.string().c_str(), "rb");
    if (!lands_shp) {
        throw std::runtime_error(
            fmt::format("failed opening lands dbf. error: {}({})", strerror(errno), errno));
    }

    double adfMinBound[4], adfMaxBound[4];
    int nShapeType, nEntities;

    SHPGetInfo(lands_shp, &nEntities, &nShapeType, adfMinBound, adfMaxBound);
    if (nShapeType != SHPT_POLYGON) {
        throw std::runtime_error(
            fmt::format("Unexpected type of shape: {}", SHPTypeName(nShapeType)));
    }

    shapes_t shapes;
    for (int i = 0; i < nEntities; i++) {
        std::unique_ptr<SHPObject, decltype(&SHPDestroyObject)> shape(SHPReadObject(lands_shp, i),
                                                                      &SHPDestroyObject);

        if (!shape) {
            throw std::runtime_error(fmt::format("failed reading shape object {}", i));
        }

        shape_parts_t shape_parts;
        for (auto part_i = 0; part_i < shape->nParts; ++part_i) {
            assert(shape->panPartType[part_i] == SHPP_RING);

            const size_t part_start_offset = shape->panPartStart[part_i];
            const size_t part_end_offset =
                part_i == shape->nParts - 1 ? shape->nVertices : shape->panPartStart[part_i + 1];

            assert(shape->padfX[part_start_offset] == shape->padfX[part_end_offset - 1]);

            part_points_t part_points;
            // todo:  detect orientation of RING.
            for (size_t vi = part_start_offset; vi != part_end_offset; ++vi) {
                auto x = shape->padfX[vi];
                auto y = shape->padfY[vi];
                x = gg::mercator::project_lon(gg::mercator::clamp_lon_to_valid(x));
                y = gg::mercator::project_lat(gg::mercator::clamp_lat_to_valid(y));
                gg::gpt_units_t ux = gg::lon_to_x(x);
                gg::gpt_units_t uy = gg::lat_to_y(y);
                part_points.emplace_back(ux, uy);
            }
            part_points = gg::utils::eliminate_parallel_segments(std::move(part_points));
            if (part_points.size() < 3) {
                log_warn("discarding part {} of shape {}: less than 3 points", i, part_i);
            } else {
                if (part_points.front() != part_points.back()) {
                    log_debug("{} != {}", part_points.front(), part_points.back());
                    assert(part_points.front() == part_points.back());
                }
                shape_parts.emplace_back(std::move(part_points));
            }
        }

        shapes.emplace_back(std::move(shape_parts));
    }

    log_debug("Loaded {} shapes:", shapes.size());
    int shape_n = 0;
    for (auto &shape : shapes) {
        size_t part_size = 0;
        for (auto &part : shape) {
            part_size += part.size();
        }
        log_debug("  {}: {} parts ({} points) ", shape_n++, shape.size(), part_size);
    }

    return std::move(shapes);
}

} // namespace map_compiler
