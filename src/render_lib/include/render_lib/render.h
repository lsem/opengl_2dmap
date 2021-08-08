#pragma once
#include <vector>

#include "i_render_unit.h"

namespace render {

class Render {
  std::vector<IRenderUnit *> m_units;

public:
  void add_unit(IRenderUnit *render_unit) { m_units.emplace_back(render_unit); }

  void render_frame() {
    for (auto *unit : m_units) {
      unit->render_frame();
    }
  }
};

} // namespace render