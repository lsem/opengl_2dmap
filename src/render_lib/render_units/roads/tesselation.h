#include "common/global.h"
#include "render_lib/i_render_unit.h"
#include "render_lib/debug_ctx.h"


namespace roads::tesselation {

struct DefaultRenderSettings {
  //inline static const double WIDTH = 20.0;
  inline static const bool GenerateBothSides = false;
  inline static const bool GenerateDebugGeometry = false;
  inline static const bool GenerateOutline = false;
};

struct FirstPassSettings {
  //inline static const double WIDTH = 2000.0;
  inline static const bool GenerateBothSides = true;
  inline static const bool GenerateDebugGeometry = false;
  inline static const bool GenerateOutline = true;
};
struct AAPassSettings {
  //inline static const double WIDTH = 300.0;
  const static bool GenerateBothSides = false;
  const static bool GenerateDebugGeometry = false;
  const static bool GenerateOutline = false;
};

/*
 For each vertex of polyline find normalized vector that bisects angle
 between them, this would give us volume for road. tip: this probably

  ------------o d
             / \
 p1 --------o p2\
           / \   \
 ---------/e  \   \
          \    \   \
           \    \p3 \

  todo: originally I wanted to find d,e as a point
  on bissectrix p1p2p3 but didn't know how to find distance,
  here is some promising simple formula:
 https://math.stackexchange.com/questions/1460994/calculating-geometry-for-a-polyline-of-a-given-thickness
  */

template <class Settings = DefaultRenderSettings>
static std::vector<p32>
generate_geometry(span<p32> polyline, span<p32> triangles_output,
                  span<p32> outline_output, double width, DebugCtx &ctx) {
  // to draw by two points, we need to have some special handling
  // which I have not implemented yet.
  assert(polyline.size() > 2);
  if (polyline.size() < 3) {
    log_warn("line with less than 3 points");
    return {{}};
  }

  // hopefully it is inlinable and thus compatible with optimizations
  auto on_outline_point = [i = 0u, &outline_output,
                           size = polyline.size()](v2 d, v2 e) mutable {
    if constexpr (Settings::GenerateOutline) {
      outline_output[i].x = d.x;
      outline_output[i].y = d.y;
      if constexpr (Settings::GenerateBothSides) {
        outline_output[size * 2 - i - 1].x = e.x;
        outline_output[size * 2 - i - 1].y = e.y;
      }
      i++;
    }
  };

  auto on_triange = [i = 0u, &triangles_output](v2 p1, v2 p2, v2 p3,
                                                v2 p4) mutable {
    if ((i + 5) >= triangles_output.size()) {
      log_err("fatal: check fialed: ({}) >= {}", i + 5,
              triangles_output.size());
    }
    assert((i + 5) < triangles_output.size());
    // generate two triangles per one quad, 6 vertex total, for using ebo we
    // could just emit four points clockwise.

    triangles_output[i + 0].x = p1.x;
    triangles_output[i + 0].y = p1.y;
    triangles_output[i + 1].x = p2.x;
    triangles_output[i + 1].y = p2.y;
    triangles_output[i + 2].x = p3.x;
    triangles_output[i + 2].y = p3.y;

    triangles_output[i + 3].x = p3.x;
    triangles_output[i + 3].y = p3.y;
    triangles_output[i + 4].x = p4.x;
    triangles_output[i + 4].y = p4.y;
    triangles_output[i + 5].x = p1.x;
    triangles_output[i + 5].y = p1.y;

    i += 6;
  };
  double FACTOR = 1000.0;

  // Process polyline by overving 3 adjacent vertices
  v2 prev_d, prev_e;
  for (size_t i = 2; i < polyline.size(); ++i) {
    auto p1 = polyline[i - 2];
    auto p2 = polyline[i - 1];
    auto p3 = polyline[i];

    // t1, t2: perpendiculars to a and b
    v2 a{p1, p2};
    v2 b{p2, p3};
    v2 t1 = normalized(v2{-a.y, a.x}) * width;
    v2 t2 = normalized(v2{-b.y, b.x}) * width;

    if constexpr (Settings::GenerateDebugGeometry) {
      ctx.add_line(p1, p2, colors::black); // original polyline
      ctx.add_line(p1, p1 + t1, colors::grey);
      ctx.add_line(p2, p2 + t1, colors::grey);
      ctx.add_line(p2, p2 + t2, colors::grey);
      ctx.add_line(p3, p3 + t2, colors::grey);

      ctx.add_line(p1, p1 + -t1, colors::grey);
      ctx.add_line(p2, p2 + -t1, colors::grey);
      ctx.add_line(p2, p2 + -t2, colors::grey);
      ctx.add_line(p3, p3 + -t2, colors::grey);
    }

    v2 d;
    if (!gg::lines_intersection(p1 + t1, p2 + t1, p3 + t2, p2 + t2, d)) {
      // in this case we can can just skip the point,
      // this must be something wrong with compilation side of the map.
      log_warn("parallel lines at {},{},{}", i - 2, i - 1, i);

      // todo: fix me.
      // that is the problem: if we emit a point in the same place
      // where previous point this would get algorithm nuts on second AA-pass.
      // possible fix is to skip such points in algorithm or emit something on
      // the same line.
      // on_outline_point(prev_d, prev_e);
      assert(false);

      continue;
    }

    // find intersecion on other side
    v2 e;

    if constexpr (Settings::GenerateBothSides) {
      bool intersect =
          gg::lines_intersection(p1 + -t1, p2 + -t1, p3 + -t2, p2 + -t2, e);
      assert(intersect); // we already checked this on main side.
    }

    if (i == 2) { // first iteration
      prev_d = p1 + t1;

      if constexpr (Settings::GenerateBothSides) {
        prev_e = p1 + -t1;
      }

      on_outline_point(prev_d, prev_e);
    }

    // generate two triangles epr one side: (p1, prev_d, d) and (p1, d, p2).
    on_triange(p1, prev_d, d, p2);

    if constexpr (Settings::GenerateBothSides) {
      on_triange(p1, prev_e, e, p2);
    }

    if constexpr (Settings::GenerateDebugGeometry) {
      ctx.add_line(p2, d, colors::green);
      ctx.add_line(prev_d, d, colors::blue);
      if constexpr (Settings::GenerateBothSides) {
        ctx.add_line(p2, e, colors::green);
        ctx.add_line(prev_e, e, colors::blue);
      }
    }

    on_outline_point(d, e);

    prev_d = d;
    prev_e = e;
  }

  // Handle end.
  auto p1 = polyline[polyline.size() - 2];
  auto p2 = polyline[polyline.size() - 1];
  v2 a{p1, p2};
  v2 perp_a = normalized(v2{-a.y, a.x}) * width;
  v2 d = p2 + perp_a;
  v2 e = p2 + -perp_a;

  on_outline_point(d, e);
  on_triange(p1, prev_d, d, p2);

  if constexpr (Settings::GenerateBothSides) {
    on_triange(p1, prev_e, e, p2);
  }

  if constexpr (Settings::GenerateDebugGeometry) {
    // ctx.add_line(p1, p2, colors::black); // original polyline
    // ctx.add_line(prev_d, d, colors::blue);
    // if constexpr (Settings::GenerateBothSides) {
    //   ctx.add_line(prev_e, e, colors::blue);
    // }
  }

  return {{}};
}

} // namespace roads::tesselation