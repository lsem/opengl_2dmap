#pragma once

#include "common/log.h"
#include "glad/glad.h"
#include "render_lib/i_render_unit.h"
#include <cstddef>
#include <gg/gg.h>
#include <tuple>
#include <vector>

#include "render_lib/shader_program.h"

class CrosshairUnit : public IRenderUnit {
    unsigned m_vao = -1;
    unsigned m_vbo = -1;
    std::unique_ptr<shader_program::ShaderProgram> m_shader = nullptr;

  public:
    bool load_shaders(std::string shaders_root) {
        auto shader = shader_program::make_from_fs_bundle(shaders_root, "crosshair/crosshair");
        if (!shader) {
            log_err("failed loading shader program for triangle");
            return false;
        }
        m_shader = std::move(shader);
        return true;
    }

    virtual void render_frame(const camera::Cam2d &camera) override {
        if (m_vao == -1) {
            float crosshair_vertices[] = {
                // first triangle
                0.5f, 0.5f, 0.0f,   // top right
                0.5f, -0.5f, 0.0f,  // bottom right
                -0.5f, 0.5f, 0.0f,  // top left
                                    // second triangle
                0.5f, -0.5f, 0.0f,  // bottom right
                -0.5f, -0.5f, 0.0f, // bottom left
                -0.5f, 0.5f, 0.0f   // top left
            };
            for (auto &x : crosshair_vertices) {
                x *= 0.01;
            }
            glGenVertexArrays(1, &m_vao);
            glGenBuffers(1, &m_vbo);
            glBindVertexArray(m_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(crosshair_vertices), &crosshair_vertices[0],
                         GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
            glBindVertexArray(0);             // unbind
        }

        glUseProgram(m_shader->id);
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
};