#pragma once

#include "common/color.h"
#include "common/global.h"
#include "common/log.h"
#include "glad/glad.h"
#include "render_lib/camera.h"
#include <cstddef>
#include <gg/gg.h>
#include <tuple>
#include <vector>

#include "render_lib/shader_program.h"

namespace polygon {

struct Vertex {
    Vertex(p32 coords) : coords(coords) {}
    p32 coords;
};

class Polygon {
    unsigned m_vao = 0;
    unsigned m_vbo = 0;
    unsigned m_ebo = 0;

    std::unique_ptr<shader_program::ShaderProgram> m_shader;
    size_t m_indices_uploaded = 0;
    double m_outline_radius = 0;
    vector<p32> m_points;
  public:
    span<p32> points();
    void load_test_data();
    void reload_test_data(double radius);
    void make_outline(vector<Vertex> &points, vector<uint32_t> &indices, double radius);
    std::pair<p32 *, double> point_at_cursor(const camera::Cam2d &cam, double mouse_xpos, double mouse_ypos);

    glm::vec2 get_center() const;
    double get_zoom() const;

    bool load_shaders(std::string shaders_root);
    void set_data(span<Vertex> vertices, span<uint32_t> indices);
    void make_buffers(span<Vertex> vertices, span<uint32_t> indices);
    void render_frame(const camera::Cam2d &cam, bool wireframe_mode);
};
} // namespace polygon