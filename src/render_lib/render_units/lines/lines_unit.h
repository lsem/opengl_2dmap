#pragma once

#include "common/color.h"
#include "common/gl_check.h"
#include "common/log.h"
#include "glad/glad.h"
#include "render_lib/i_render_unit.h"
#include "render_lib/shader_program.h"
#include <cstddef>
#include <gg/gg.h>
#include <tuple>
#include <vector>

class LinesUnit : public IRenderUnit {
    std::vector<float> m_geometry;
    std::vector<float> m_colors;
    unsigned m_vao = -1;
    unsigned m_vbo = -1;
    unsigned m_colors_vbo = -1;
    std::unique_ptr<shader_program::ShaderProgram> m_shader = nullptr;
    bool m_dirty = true;

  public:
    using line_type = std::tuple<gg::v2, gg::v2, Color>;

    void assign_lines(std::vector<line_type> lines) {
        assert(m_geometry.capacity() == 0);
        assert(m_colors.capacity() == 0);
        m_geometry.reserve(lines.size() * 4);
        m_colors.reserve(lines.size() * 6);
        m_geometry.clear();
        m_colors.clear();
        for (auto [a, b, color] : lines) {
            m_geometry.push_back(a.x);
            m_geometry.push_back(a.y);
            m_geometry.push_back(b.x);
            m_geometry.push_back(b.y);
            // line has two vertex colored one way, no interpolation needed
            m_colors.push_back(color.r);
            m_colors.push_back(color.g);
            m_colors.push_back(color.b);
            m_colors.push_back(color.r);
            m_colors.push_back(color.g);
            m_colors.push_back(color.b);
        }
        m_dirty = true;
    }

    void create_buffers() {
        // todo: what if I can add more data by putthing new data into separate
        // vertex object and somehow record this info in VAO?
        assert(m_vao == -1);
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_colors_vbo);
        glBindVertexArray(m_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, m_geometry.size() * sizeof(m_geometry[0]), NULL,
                     GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void *)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind

        // color
        glBindBuffer(GL_ARRAY_BUFFER, m_colors_vbo);
        glBufferData(GL_ARRAY_BUFFER, m_colors.size() * sizeof(m_colors[0]), NULL, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *)0);
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind

        glBindVertexArray(0); // unbind
    }

    bool reupload_geometry() {
        if (m_vao == -1) {
            if (m_geometry.empty())
                return false;
            create_buffers();
        }
        assert(m_vao != -1);

        // log_debug("reuploaded {} vertices", m_geometry.size());

        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, this->m_vbo));
        GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, m_geometry.size() * sizeof(m_geometry[0]),
                                 m_geometry.data()));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0)); // unbind

        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, this->m_colors_vbo));
        GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, m_colors.size() * sizeof(m_colors[0]),
                                 m_colors.data()));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0)); // unbind

        return true;
    }

    bool load_shaders(std::string shaders_root) {
        auto shader = shader_program::make_from_fs_bundle(shaders_root, "lines/lines");
        if (!shader) {
            log_err("failed loading shader program for triangle");
            return false;
        }
        m_shader = std::move(shader);
        return true;
    }

    virtual void render_frame(const camera::Cam2d &camera) override {
        if (!m_shader) {
            log_err("lines shader not ready");
            return;
        }

        if (m_dirty) {
            if (!reupload_geometry())
                return;
            m_dirty = false;
        }
        GL_CHECK(glUseProgram(m_shader->id));
        auto proj = camera.projection_maxtrix();
        GL_CHECK(glUniformMatrix4fv(glGetUniformLocation(m_shader->id, "proj"), 1, GL_FALSE,
                                    glm::value_ptr(proj)));
        GL_CHECK(glBindVertexArray(m_vao));
        GL_CHECK(glDrawArrays(GL_LINES, 0, m_geometry.size() / 2));
    }
};
