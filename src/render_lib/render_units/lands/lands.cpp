#include "lands.h"
#include <common/gl_check.h>
#include <glm/gtc/type_ptr.hpp>

namespace lands {

bool Lands::load_shaders(std::string shaders_root) {
    auto shader = shader_program::make_from_fs_bundle(shaders_root, "lands/lands");
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

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
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

bool Lands::make_buffers() {
    GL_CHECK(glGenVertexArrays(1, &m_vao));
    GL_CHECK(glGenBuffers(1, &m_vbo));
    GL_CHECK(glGenBuffers(1, &m_ebo));

    GL_CHECK(glBindVertexArray(m_vao));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, 20'000'000, NULL, GL_DYNAMIC_DRAW));

    GL_CHECK(glVertexAttribPointer(0, 2, GL_UNSIGNED_INT, GL_FALSE, sizeof(p32), (void *)0));
    GL_CHECK(glEnableVertexAttribArray(0));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 20'000'000, NULL, GL_DYNAMIC_DRAW));
    // DO NOT UNBIND EBO!

    GL_CHECK(glBindVertexArray(0)); // unbind vao.

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
    GL_CHECK(glBindVertexArray(m_vao));
    GL_CHECK(glDrawElements(GL_TRIANGLES, m_indices_uploaded, GL_UNSIGNED_INT, 0));
    GL_CHECK(glBindVertexArray(0));
    m_shader->detach();
}

} // namespace lands