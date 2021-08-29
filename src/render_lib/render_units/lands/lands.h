#pragma once

#include "common/global.h"

#include "render_lib/i_render_unit.h"
#include "render_lib/shader_program.h"

namespace lands {
class Lands : public IRenderUnit {
    unsigned m_vao = 0;
    unsigned m_vbo = 0;
    unsigned m_ebo = 0;
    size_t m_vertices_uploaded = 0;
    size_t m_indices_uploaded = 0;
    std::unique_ptr<shader_program::ShaderProgram> m_shader;

  public:
    bool load_shaders(std::string shaders_root);
    void set_data(span<p32> aa_vertex_data, span<uint32_t> aa_vertex_indices);
    bool make_buffers();
    virtual void render_frame(const camera::Cam2d &cam) override;
};
} // namespace lands