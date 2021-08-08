#pragma once

#include "glad/glad.h"
#include "common/log.h"
#include <cstddef>

#include "render_lib/i_render_unit.h"

#include "render_lib/shader_program.h"

namespace {
#pragma pack(0)
struct Vertex { // todo: is it packed?
  Vertex(float x, float y, float z, float col) : x(x), y(y), z(z), col(col) {}
  float x, y, z, col;
};
Vertex vertices3[] = {{10000.0, 20000.0, 0.0f, 1.0f},
                      {10100.0, 20100.0, 0.0f, 0.5f},
                      {10000.0, 20200.0, 0.0f, 0.0f}};

} // namespace

class TriangleUnit : public IRenderUnit {
  unsigned m_vao = -1;
  unsigned m_vbo = -1;
  std::unique_ptr<shader_program::ShaderProgram> m_shader = nullptr;

public:
  glm::vec2 triangle_center() const {
    float cx = 0.0, cy = 0.0;
    for (auto &v : vertices3) {
      cx += v.x, cy += v.y;
    }
    cx /= 3.0, cy /= 3.0;
    return glm::vec2{cx, cy};
  }

  bool load_shaders(std::string shaders_root) {
    auto shader =
        shader_program::make_from_fs_bundle(shaders_root, "triangle/triangle");
    if (!shader) {
      log_err("failed loading shader program for triangle");
      return false;
    }
    m_shader = std::move(shader);
    return true;
  }

  void upload_geometry() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices3), &vertices3[0],
                 GL_STATIC_DRAW);
    // data vec3
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    // color
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, col));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
    glBindVertexArray(0);             // unbind
  }

  virtual void render_frame(const camera::Cam2d &camera) override {
    if (!m_shader) {
      log_err("triangle shader not ready");
      return;
    }

    glUseProgram(m_shader->id);
    auto proj = camera.projection_maxtrix();
    glUniformMatrix4fv(glGetUniformLocation(m_shader->id, "proj"), 1, GL_FALSE,
                       glm::value_ptr(proj));
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
};
