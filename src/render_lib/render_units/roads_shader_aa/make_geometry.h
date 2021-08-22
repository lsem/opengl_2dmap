#include "common/global.h"
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
static std::tuple<size_t, size_t> make_geometry(span<p32> polyline,
                                                span<AAVertex> out_vertices,
                                                span<uint32_t> out_indices,
                                                double width) {
  // to draw by two points, we need to have some special handling
  // which I have not implemented yet.
  assert(polyline.size() > 2);
  if (polyline.size() < 3) {
    log_warn("line with less than 3 points");
    return {-1, -1};
  }

  size_t vi = 0;
  size_t ii = 0;

  auto on_first_pair = [&](v2 p, v2 d) {
    out_vertices[vi].coords.x = static_cast<uint32_t>(std::round(p.x));
    out_vertices[vi].coords.y = static_cast<uint32_t>(std::round(p.y));
    out_vertices[vi].is_outer = 0;

    // we don't use d point here but basically give d point p's cooridinates
    // so that by adding extent_vec it should be d. The shader will add its
    // outer coordinate with extent_vec and it will be what d is in here.
    auto v = v2(p, d);
    out_vertices[vi + 1].extent_vec[0] = v.x;
    out_vertices[vi + 1].extent_vec[1] = v.y;
    out_vertices[vi + 1].coords.x = static_cast<uint32_t>(std::round(p.x));
    out_vertices[vi + 1].coords.y = static_cast<uint32_t>(std::round(p.y));
    out_vertices[vi + 1].is_outer = 1;

    vi += 2;
  };

  auto on_next_pair = [&](v2 p, v2 d) {
    // -----------------------------------
    //
    //  prev_d            d
    //  o----------------o
    //  |              /  \
    //  o------------o     \
    //  p1         p2 \
    //
    // -----------------------------------
    // v2 v(p, d);
    // now, vertex d needs vector pd and verex p2 needs just color.
    // d must be

    out_vertices[vi].coords.x = static_cast<uint32_t>(std::round(p.x));
    out_vertices[vi].coords.y = static_cast<uint32_t>(std::round(p.y));
    out_vertices[vi].is_outer = 0;
    // out_vertices[vi + 1].coords.x = static_cast<uint32_t>(std::round(d.x));
    // out_vertices[vi + 1].coords.y = static_cast<uint32_t>(std::round(d.y));
    out_vertices[vi + 1].coords.x = static_cast<uint32_t>(std::round(p.x));
    out_vertices[vi + 1].coords.y = static_cast<uint32_t>(std::round(p.y));

    out_vertices[vi + 1].is_outer = 1;

    // v2 dp = gg::normalized(v2(d, p));
    out_vertices[vi + 1].extent_vec[0] = v2(p, d).x;
    out_vertices[vi + 1].extent_vec[1] = v2(p, d).y;

    vi += 2;

    size_t i_p1 = vi - 4;
    size_t i_prev_d = vi - 3;
    size_t i_d = vi - 1;
    size_t i_p2 = vi - 2;

    out_indices[ii++] = i_p1;
    out_indices[ii++] = i_prev_d;
    out_indices[ii++] = i_d;
    out_indices[ii++] = i_p1;
    out_indices[ii++] = i_d;
    out_indices[ii++] = i_p2;
  };

  // Process polyline by overving 3 adjacent vertices
  v2 prev_d;
  for (size_t i = 2; i < polyline.size(); ++i) {
    v2 p1(polyline[i - 2]);
    v2 p2(polyline[i - 1]);
    v2 p3(polyline[i]);

    // t1, t2: perpendiculars to a and b
    v2 a(p1, p2);
    v2 b(p2, p3);
    v2 t1 = normalized(v2(-a.y, a.x)) * width;
    v2 t2 = normalized(v2(-b.y, b.x)) * width;

    v2 d;
    if (unlikely(!gg::lines_intersection(p1 + t1, p2 + t1, p3 + t2, p2 + t2, d))) {
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

    //  prev_d       d
    //  o------------o
    //  |            |
    //  o------------o
    //  p1          p2

    if (unlikely(i == 2)) { // first iteration
      prev_d = p1 + t1;
      // on_first_pair(p1, prev_d);
      on_first_pair(p1, prev_d);
    }

    on_next_pair(p2, d);

    prev_d = d;
  }

  // Handle end.
  auto p1 = polyline[polyline.size() - 2];
  auto p2 = polyline[polyline.size() - 1];
  v2 a(p1, p2);
  v2 perp_a = normalized(v2(-a.y, a.x)) * width;
  v2 d = p2 + perp_a;

  on_next_pair(p2, d);

  return tuple{vi, ii};
}

} // namespace roads_shader_aa