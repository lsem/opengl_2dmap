#include "common/global.h"
#include "render_lib/debug_ctx.h"
#include "types.h"

namespace roads_shader_aa {

#define likely(x) x
#define unlikely(x) x

/*
  ------------o d
             / \
 p1 --------o p2\
           / \   \
 ---------/e  \   \
          \    \   \
           \    \p3 \
*/
// Generate aa geometry and data needed for shader needed for polylong aa.
template <class EventHandler> struct ExtrudePolyline : public EventHandler {
  template <typename... Args>
  ExtrudePolyline(Args... args) : EventHandler(args...) {}

  void extrude_polyline(span<p32> polyline, double width, DebugCtx &debug_ctx) {
    assert(polyline.size() > 2);
    if (polyline.size() < 3) {
      log_err("line with less than 3 points");
      return;
    }

    bool was_parallel = false;
    const size_t N = polyline.size();
    v2 prev_d;
    for (size_t i = 0; i < N; ++i) {
      v2 p1(polyline[(i + N - 1) % N]);
      v2 p2(polyline[i]);
      v2 p3(polyline[(i + 1) % N]);
      v2 a(p1, p2);
      v2 b(p2, p3);
      v2 t1 = normalized(v2(-a.y, a.x)) * width;
      v2 t2 = normalized(v2(-b.y, b.x)) * width;
      v2 d;
      if (unlikely(
              !gg::lines_intersection(p1 + t1, p2 + t1, p3 + t2, p2 + t2, d))) {
        auto l1 = len(v2(p1 + t1, p2 + t1));
        auto l2 = len(v2(p3 + t2, p2 + t2));
        debug_ctx.add_line(p1 + t1, p2 + t1, colors::red, "parallel-lines");
        debug_ctx.add_line(p3 + t2, p2 + t2, colors::blue, "parallel-lines");
        log_warn(
            "parallel lines at {},{},{} ({} / {} / {} / {}) (lengths: {}, {})",
            i - 2, i - 1, i, p1 + t1, p2 + t1, p3 + t2, p2 + t2, l1, l2);
        assert(false);
      }
      EventHandler::next(p2, d);
      prev_d = d;
    }
    EventHandler::finish();
  }
};

struct PolylineAAHandler {
  span<AAVertex> out_vertices;
  span<uint32_t> out_indices;
  size_t vi;
  size_t ii;

  PolylineAAHandler(span<AAVertex> out_vertices, span<uint32_t> out_indices)
      : out_vertices(out_vertices), out_indices(out_indices), vi(0), ii(0) {}

  void add_quad(size_t i_p1, size_t i_prev_d, size_t i_d, size_t i_p2) {
    out_indices[ii++] = i_p1;
    out_indices[ii++] = i_prev_d;
    out_indices[ii++] = i_d;
    out_indices[ii++] = i_p1;
    out_indices[ii++] = i_d;
    out_indices[ii++] = i_p2;
  }

  void next(v2 p, v2 d) {
    assert((out_vertices.size() - vi) > 2);
    assert((out_indices.size() - ii) > 6);
    // -----------------------------------
    //
    //  prev_d            d
    //  o----------------o
    //  |            v /  \
    //  o------------o     \
    //  p1         p2 \
    //
    // -----------------------------------

    out_vertices[vi].coords.x = static_cast<uint32_t>(std::round(p.x));
    out_vertices[vi].coords.y = static_cast<uint32_t>(std::round(p.y));
    out_vertices[vi].is_outer = 0;

    // we don't use d point here but basically give d point p's cooridinates
    // so that by adding extent_vec it should be d. The shader will add its
    // outer coordinate with extent_vec and it will be what d is in here.
    out_vertices[vi + 1].coords.x = static_cast<uint32_t>(std::round(p.x));
    out_vertices[vi + 1].coords.y = static_cast<uint32_t>(std::round(p.y));
    out_vertices[vi + 1].is_outer = 1;
    auto v = v2(p, d);
    out_vertices[vi + 1].extent_vec[0] = v.x;
    out_vertices[vi + 1].extent_vec[1] = v.y;
    vi += 2;

    if (likely(vi > 2)) {
      // first point just collects vertices but does not produce triangles yet
      // since there is no previous point.
      add_quad(vi - 4, vi - 3, vi - 1, vi - 2);
    }
  }

  void finish() { // connect last point to first.
    add_quad(vi - 2, vi - 1, 1, 0);
  }
};

static std::tuple<size_t, size_t>
make_geometry(span<p32> polyline, double width, span<AAVertex> out_vertices,
              span<uint32_t> out_indices, DebugCtx &debug_ctx) {
  ExtrudePolyline<PolylineAAHandler> extrude(out_vertices, out_indices);
  extrude.extrude_polyline(polyline, width, debug_ctx);
  return {extrude.vi, extrude.ii};
}

} // namespace roads_shader_aa