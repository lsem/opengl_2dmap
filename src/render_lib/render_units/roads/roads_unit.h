#pragma once

#include "common/global.h"
#include "common/log.h"
#include "glad/glad.h"
#include "render_lib/i_render_unit.h"
#include <cstddef>
#include <gg/gg.h>
#include <tuple>
#include <vector>

#include "render_lib/shader_program.h"

class RoadsUnit : public IRenderUnit {
  unsigned m_vao = 0;
  unsigned m_vbo =  0;
  unsigned m_ebo;
  size_t m_vertices_uploaded = 0;
  std::unique_ptr<shader_program::ShaderProgram> m_shader = nullptr;
  // This unit is not going to own its resources directly?
public:
  bool load_shaders(std::string shaders_root);
  void set_data(span<p32> vertex_data);
  bool make_buffers(); // todo: should not be part of interface.
  virtual void render_frame(const camera::Cam2d &cam) override;
};
