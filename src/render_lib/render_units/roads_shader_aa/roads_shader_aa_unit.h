#pragma once

#include "common/color.h"
#include "common/global.h"
#include "common/log.h"
#include "glad/glad.h"
#include "render_lib/i_render_unit.h"
#include <cstddef>
#include <gg/gg.h>
#include <tuple>
#include <vector>

#include "render_lib/shader_program.h"
#include "types.h"

namespace roads_shader_aa {
class RoadsShaderAAUnit : public IRenderUnit {
  unsigned m_vao = 0;
  unsigned m_vbo = 0;
  unsigned m_ebo = 0;

  size_t m_vertices_uploaded = 0;
  size_t m_indices_uploaded = 0;
  std::unique_ptr<shader_program::ShaderProgram> m_shader = nullptr;

public:
  bool load_shaders(std::string shaders_root);
  void set_data(span<AAVertex> aa_vertex_data,
                span<uint32_t> aa_vertex_indices);
  bool make_buffers(); // todo: should not be part of interface.
  virtual void render_frame(const camera::Cam2d &cam) override;
};
} // namespace roads_shader_aa