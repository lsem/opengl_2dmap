#include "lands.h"
#include <glm/gtc/type_ptr.hpp>

namespace lands {

bool Lands::load_shaders(std::string shaders_root) {
  auto shader =
      shader_program::make_from_fs_bundle(shaders_root, "lands/lands");
  if (!shader) {
    log_err("failed loading shader program for lands");
    return false;
  }
  m_shader = std::move(shader);
  return true;
}

void Lands::set_data(span<p32> vertices, span<uint32_t> indices) {
  assert(m_vbo != 0);
  assert(m_ebo != 0);

  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(vertices[0]),
                  vertices.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  if (glGetError() != GL_NO_ERROR) {
    log_err("lands vertices set_data error: {}", glGetError());
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                  indices.size() * sizeof(indices[0]), indices.data());
  if (glGetError() != GL_NO_ERROR) {
    log_err("lands indices set_data error: {}", glGetError());
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  m_vertices_uploaded = vertices.size();
  m_indices_uploaded = indices.size();
}

bool Lands::make_buffers() {
  glGenVertexArrays(1, &m_vao);
  glGenBuffers(1, &m_vbo);
  glGenBuffers(1, &m_ebo);

  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, 10'000'000, NULL, GL_DYNAMIC_DRAW);
  if (glGetError() != GL_NO_ERROR) {
    log_err("lands vertices alloc error: {}", glGetError());
  }

  glVertexAttribPointer(0, 2, GL_UNSIGNED_INT, GL_FALSE, sizeof(p32),
                        (void *)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 10'000'000, NULL, GL_DYNAMIC_DRAW);
  if (glGetError() != GL_NO_ERROR) {
    log_err("lands indices alloc error: {}", glGetError());
  }
  // DO NOT UNBIND EBO!

  glBindVertexArray(0); // unbind vao.

  return true;
}

void Lands::render_frame(const camera::Cam2d &cam) {
  if (!m_shader) {
    static bool first = true;
    if (first) {
      log_err("lands: Shader not created");
      first = false;
    }
    return;
  }
  m_shader->attach();
  auto proj = cam.projection_maxtrix();
  glUniformMatrix4fv(glGetUniformLocation(m_shader->id, "proj"), 1, GL_FALSE,
                     glm::value_ptr(proj));
  glBindVertexArray(m_vao);
  glDrawElements(GL_TRIANGLES, m_indices_uploaded, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
  m_shader->detach();
}

} // namespace lands