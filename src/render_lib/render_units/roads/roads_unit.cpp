#include "roads_unit.h"
//#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
const size_t RESERVED_VERTEX_DATA_SIZE = 10 * 1024 * 1024;
}

bool RoadsUnit::load_shaders(std::string shaders_root) {
  auto shader =
      shader_program::make_from_fs_bundle(shaders_root, "roads/roads");
  if (!shader) {
    log_err("failed loading shader program for roads");
    return false;
  }
  auto aa_shader =
      shader_program::make_from_fs_bundle(shaders_root, "roads/roads_aa");
  if (!aa_shader) {
    log_err("failed loading shader program for roads aa");
    return false;
  }

  m_shader = std::move(shader);
  m_aa_shader = std::move(aa_shader);

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

  glGenVertexArrays(1, &m_aa_vao);
  glGenBuffers(1, &m_aa_vbo);
  glBindVertexArray(m_aa_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_aa_vbo);
  const size_t aa_one_triangle_size =
      6 * (2 * sizeof(uint32_t) + 4 * sizeof(float)); // one vertex is 2 coords + 4 color components, 6 vertices in total.
  glBufferData(GL_ARRAY_BUFFER, aa_one_triangle_size * 100'000, nullptr,
               GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 2, GL_UNSIGNED_INT, GL_FALSE, sizeof(ColoredVertex),
                        (void *)offsetof(ColoredVertex, vertex));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex),
                        (void *)offsetof(ColoredVertex, color));
  glEnableVertexAttribArray(1);
  glBindVertexArray(0); // unbind aa vao

  return true;
}

void RoadsUnit::set_data(span<p32> vertex_data,
                         span<ColoredVertex> aa_vertex_data) {
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0,
                  vertex_data.size() * sizeof(vertex_data[0]),
                  vertex_data.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
  m_vertices_uploaded = vertex_data.size();
  log_debug("road triangles vertices count: {}", m_vertices_uploaded);

  glBindBuffer(GL_ARRAY_BUFFER, m_aa_vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0,
                  aa_vertex_data.size() * sizeof(aa_vertex_data[0]),
                  aa_vertex_data.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
  m_aa_vertices_uploaded = aa_vertex_data.size();
  log_debug("road aa triangles vertices count: {}", m_aa_vertices_uploaded);  
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

  m_aa_shader->attach();
  glUniformMatrix4fv(glGetUniformLocation(m_aa_shader->id, "proj"), 1, GL_FALSE,
                     glm::value_ptr(proj));
  glBindVertexArray(m_aa_vao);
  glDrawArrays(GL_TRIANGLES, 0, m_aa_vertices_uploaded);
  glBindVertexArray(0);
  m_aa_shader->detach();
}
