#include "animatable_line.h"

#include <common/gl_check.h>
#include <glm/gtc/type_ptr.hpp>

namespace animatable_line {
bool AnimatableLine::load_shaders(std::string shaders_root) {
    auto shader =
        shader_program::make_from_fs_bundle(shaders_root, "animatable_line/animatable_line");
    if (!shader) {
        log_err("failed loading shader program for animatable line");
        return false;
    }

    m_shader = std::move(shader);

    return true;
}

bool AnimatableLine::make_buffers() {
    GL_CHECK(glGenVertexArrays(1, &m_vao));
    GL_CHECK(glGenBuffers(1, &m_vbo));
    GL_CHECK(glGenBuffers(1, &m_ebo));

    GL_CHECK(glBindVertexArray(m_vao));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, 100'000'000, NULL, GL_DYNAMIC_DRAW));

    static_assert(sizeof(p32) == sizeof(uint32_t) * 2); // todo: fix me.
    GL_CHECK(glVertexAttribPointer(0, 2, GL_UNSIGNED_INT, GL_FALSE, sizeof(Vertex),
                                   (void *)offsetof(Vertex, coords)));
    GL_CHECK(glVertexAttribIPointer(1, 1, GL_BYTE, sizeof(Vertex), (void *)offsetof(Vertex, t)));

    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glEnableVertexAttribArray(1));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 60'000'000, NULL, GL_DYNAMIC_DRAW));
    // DO NOT UNBIND EBO!

    GL_CHECK(glBindVertexArray(0)); // unbind vao.

    m_vertices_uploaded = 0;
    m_indices_uploaded = 0;

    return true;
}

void AnimatableLine::set_data(span<Vertex> vertices, span<uint32_t> indices) {
    assert(m_vbo != 0);
    assert(m_ebo != 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(vertices[0]),
                             vertices.data()));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo));
    GL_CHECK(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(indices[0]),
                             indices.data()));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

    m_vertices_uploaded = vertices.size();
    m_indices_uploaded = indices.size();
}

/*virtual*/
void AnimatableLine::render_frame(const camera::Cam2d &cam) /*override*/ {
    m_shader->attach();
    auto proj = cam.projection_maxtrix();
    GL_CHECK(glUniformMatrix4fv(glGetUniformLocation(m_shader->id, "proj"), 1, GL_FALSE,
                                glm::value_ptr(proj)));
    GL_CHECK(glUniform1f(glGetUniformLocation(m_shader->id, "scale"), (float)cam.zoom));

    unsigned data[100];
    for (size_t i = 0; i < 100; ++i) {
        data[i] = i * 10;
    }

    GL_CHECK(glUniform1uiv(glGetUniformLocation(m_shader->id, "data"), 100, &data[0]));

    // log_debug("drawing: {}", m_indices_uploaded);

    GL_CHECK(glBindVertexArray(m_vao));

    GL_CHECK(glDrawElements(GL_TRIANGLES, m_indices_uploaded, GL_UNSIGNED_INT, 0));
    glBindVertexArray(0);
    m_shader->detach();
}
} // namespace animatable_line