#include "roads_unit.h"
//#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
const size_t RESERVED_VERTEX_DATA_SIZE = 100 * 1024 * 1024;
}

bool RoadsUnit::load_shaders(std::string shaders_root) {
  auto shader =
      shader_program::make_from_fs_bundle(shaders_root, "roads/roads");
  if (!shader) {
    log_err("failed loading shader program for roads");
    return false;
  }

  m_shader = std::move(shader);

  return true;
}

bool RoadsUnit::make_buffers() {
  glGenVertexArrays(1, &m_vao);
  glGenBuffers(1, &m_vbo);
  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  // Lets preallocate a bunch of data for rendering some number of roads.
  // lets preallocate 10mb first.
  // 10MB is 164k of 2d vertices
  glBufferData(GL_ARRAY_BUFFER, RESERVED_VERTEX_DATA_SIZE, NULL,
               GL_DYNAMIC_DRAW);
  static_assert(sizeof(p32) == sizeof(uint32_t) * 2); // todo: fix me.
  glVertexAttribPointer(0, 2, GL_UNSIGNED_INT, GL_FALSE, sizeof(uint32_t) * 2,
                        (void *)0);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);

  return true;
}

void RoadsUnit::set_data(span<p32> vertex_data) {
  auto upload_start_time = std::chrono::steady_clock::now();

  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0,
                  vertex_data.size() * sizeof(vertex_data[0]),
                  vertex_data.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
  m_vertices_uploaded = vertex_data.size();
}

/*virtual*/
void RoadsUnit::render_frame(const camera::Cam2d &cam) /*override*/ {
  m_shader->attach();
  auto proj = cam.projection_maxtrix();
  glUniformMatrix4fv(glGetUniformLocation(m_shader->id, "proj"), 1, GL_FALSE,
                     glm::value_ptr(proj));
  glBindVertexArray(m_vao);
  glDrawArrays(GL_TRIANGLES, 0, m_vertices_uploaded);
  glBindVertexArray(0);
  m_shader->detach();
}
