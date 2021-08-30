#pragma once

#include "common/color.h"
#include "common/global.h"
#include "common/log.h"
#include "glad/glad.h"
#include "render_lib/i_render_unit.h"
#include <cstddef>
#include <gg/gg.h>
#include <tuple>
#include <vector>

#include "render_lib/shader_program.h"

namespace animatable_line {

struct Vertex {
    Vertex(p32 coords, float t) : coords(coords), t(t) {}
    p32 coords;
    float t;
};

class AnimatableLine : public IRenderUnit {
    unsigned m_vao = 0;
    unsigned m_vbo = 0;
    unsigned m_ebo = 0;

    size_t m_vertices_uploaded = 0;
    size_t m_indices_uploaded = 0;
    std::unique_ptr<shader_program::ShaderProgram> m_shader;

  public:
    bool load_shaders(std::string shaders_root);
    void set_data(span<Vertex> vertices, span<uint32_t> indices);
    bool make_buffers();
    virtual void render_frame(const camera::Cam2d &cam) override;
};
} // namespace animatable_line