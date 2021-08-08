#include "common/color.h"
#include "common/global.h"

namespace {
p32 from_v2(v2 v) {
  return p32{static_cast<uint32_t>(v.x), static_cast<uint32_t>(v.y)};
}
} // namespace

struct DebugCtx {
  std::vector<tuple<p32, p32, Color>> lines;
  // if we add key then we can have layered structure with checkboxes in imgui.
  void add_line(p32 a, p32 b, Color c) { this->lines.emplace_back(a, b, c); }
  void add_line(v2 a, v2 b, Color c) {
    this->lines.emplace_back(from_v2(a), from_v2(b), c);
  }
};