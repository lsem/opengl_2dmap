#include "roads_shader_aa_unit.h"
//#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace roads_shader_aa {
bool RoadsShaderAAUnit::load_shaders(std::string shaders_root) {
  auto shader = shader_program::make_from_fs_bundle(shaders_root,
                                                    "roads_shader_aa/roads");
  if (!shader) {
    log_err("failed loading shader program for roads");
    return false;
  }

  m_shader = std::move(shader);

  return true;
}

bool RoadsShaderAAUnit::make_buffers() {
  glGenVertexArrays(1, &m_vao);
  glGenBuffers(1, &m_vbo);
  glGenBuffers(1, &m_ebo);

  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, 100'000, NULL, GL_DYNAMIC_DRAW);
  static_assert(sizeof(p32) == sizeof(uint32_t) * 2); // todo: fix me.
  glVertexAttribPointer(0, 2, GL_UNSIGNED_INT, GL_FALSE, sizeof(AAVertex),
                        (void *)offsetof(AAVertex, coords));
  glVertexAttribIPointer(1, 1, GL_BYTE, sizeof(AAVertex),
                         (void *)offsetof(AAVertex, is_outer));
  // glVertexAttribIPointer(2, 3, GL_UNSIGNED_BYTE, sizeof(AAVertex),
  //                        (void *)offsetof(AAVertex, color));
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(AAVertex),
                         (void *)offsetof(AAVertex, color));

  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(AAVertex),
                        (void *)offsetof(AAVertex, extent_vec));

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(3);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 100'000, NULL, GL_DYNAMIC_DRAW);
  // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  glBindVertexArray(0); // unbind vao.

  // glGenVertexArrays(1, &m_aa_vao);
  // glGenBuffers(1, &m_aa_vbo);
  // glBindVertexArray(m_aa_vao);
  // glBindBuffer(GL_ARRAY_BUFFER, m_aa_vbo);
  // const size_t aa_one_triangle_size =
  //     6 * (2 * sizeof(uint32_t) +
  //          4 * sizeof(float)); // one vertex is 2 coords + 4 color
  //          components, 6
  //                              // vertices in total.
  // glBufferData(GL_ARRAY_BUFFER, aa_one_triangle_size * 100'000'00, nullptr,
  //              GL_DYNAMIC_DRAW);
  // glVertexAttribPointer(0, 2, GL_UNSIGNED_INT, GL_FALSE,
  // sizeof(ColoredVertex),
  //                       (void *)offsetof(ColoredVertex, vertex));
  // glEnableVertexAttribArray(0);
  // glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex),
  //                       (void *)offsetof(ColoredVertex, color));
  // glEnableVertexAttribArray(1);
  // glBindVertexArray(0); // unbind aa vao

  m_vertices_uploaded = 0;
  m_indices_uploaded = 0;

  return true;
}

void RoadsShaderAAUnit::set_data(span<AAVertex> aa_vertices,
                                 span<uint32_t> aa_indices) {
  assert(m_vbo != 0);
  assert(m_ebo != 0);

  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0,
                  aa_vertices.size() * sizeof(aa_vertices[0]),
                  aa_vertices.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                  aa_indices.size() * sizeof(aa_indices[0]), aa_indices.data());
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  m_vertices_uploaded = aa_vertices.size();
  m_indices_uploaded = aa_indices.size();
}

/*virtual*/
void RoadsShaderAAUnit::render_frame(const camera::Cam2d &cam) /*override*/ {
  m_shader->attach();
  auto proj = cam.projection_maxtrix();
  glUniformMatrix4fv(glGetUniformLocation(m_shader->id, "proj"), 1, GL_FALSE,
                     glm::value_ptr(proj));
  glUniform1f(glGetUniformLocation(m_shader->id, "scale"), (float)cam.zoom);

  unsigned data[100];
  for (size_t i = 0; i < 100; ++i) {
    data[i] = i * 10;
  }

  glUniform1uiv(glGetUniformLocation(m_shader->id, "data"), 100, &data[0]);

  glBindVertexArray(m_vao);
  glDrawElements(GL_TRIANGLES, m_indices_uploaded, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
  m_shader->detach();
}
} // namespace roads_shader_aa