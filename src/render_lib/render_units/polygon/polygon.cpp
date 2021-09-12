#include "polygon.h"

#include <common/gl_check.h>
#include <glm/gtc/type_ptr.hpp>
#include "gg/math.h"

template <typename C> size_t buffer_size(const C &c) { return c.size() * sizeof(C::value_type); }

namespace polygon {

glm::vec2 Polygon::get_center() const { return glm::vec2(2288259584, 2415557120); }
double Polygon::get_zoom() const { return 1.6602280912066504e-06; }

void Polygon::load_test_data() {
    m_points = {p32(2337951488, 2338459392), p32(2202427904, 2344482560),
                p32(2203030272, 2464947968), p32(2296390912, 2418568704),
                p32(2458416896, 2490245632)};
    reload_test_data(10);
}

void Polygon::reload_test_data(double radius) {
    if (radius == m_outline_radius)
        return;
    vector<Vertex> points;
    points.assign(m_points.begin(), m_points.end());
    vector<uint32_t> indices = {0, 1, 2, 0, 2, 3, 0, 3, 4};

    make_outline(points, indices, 1'000'000 * radius);
    set_data(points, indices);
    m_outline_radius = radius;
}

void Polygon::make_outline(vector<Vertex> &points, vector<uint32_t>& indices, double radius) {
    auto make_point = [](const p32 &p) { return point2d(p.x, p.y); };
    auto make_p32 = [](const point2d &p) { return p32{(uint32_t)p.x, (uint32_t)p.y}; };
    uint32_t points_size = points.size();
    for (size_t i = 0; i < points_size; ++i) {
        uint32_t index_a = i;
        uint32_t index_b = (i + 1) % points_size;
        point2d a = make_point(points[index_a].coords);
        point2d b = make_point(points[index_b].coords);        
        auto n = radius * normal(a, b);
        uint32_t index_c = points.size();
        uint32_t index_d = points.size() + 1;
        points.push_back(make_p32(a + n));
        points.push_back(make_p32(b + n));
        indices.push_back(index_a);
        indices.push_back(index_b);
        indices.push_back(index_c);
        indices.push_back(index_b);
        indices.push_back(index_d);
        indices.push_back(index_c);
    }
}

span<p32> Polygon::points() { return m_points; }

std::pair<p32 *, double> Polygon::point_at_cursor(const camera::Cam2d &cam, double mouse_xpos,
                                                  double mouse_ypos) {
    glm::mat4 m = cam.projection_maxtrix();
    double w = cam.window_size.x;
    double h = cam.window_size.y;
    double min_dist = 1e99;
    p32 *nearest_pt = nullptr; 
    point2d mouse_pos = {mouse_xpos, mouse_ypos};
    for (auto& p : points()) {
        glm::vec4 v = {p.x, p.y, 0, 1};
        auto projected = m * v;
        point2d screen_p = {(projected.x + 1) / 2 * w, (-projected.y + 1) / 2 * h};
        auto d = dist(mouse_pos, screen_p);
        if (d < min_dist) {
            min_dist = d;
            nearest_pt = &p;
        }                    
    }
    return {nearest_pt, min_dist};
}

bool Polygon::load_shaders(std::string shaders_root) {
    auto shader = shader_program::make_from_fs_bundle(shaders_root, "polygon/polygon");
    if (!shader) {
        log_err("failed loading shader program for polygon");
        return false;
    }
    m_shader = std::move(shader);
    return true;
}

void Polygon::make_buffers(span<Vertex> vertices, span<uint32_t> indices) {
    assert(m_vbo == 0);
    assert(m_ebo == 0);

    GL_CHECK(glGenVertexArrays(1, &m_vao));
    GL_CHECK(glGenBuffers(1, &m_vbo));
    GL_CHECK(glGenBuffers(1, &m_ebo));

    GL_CHECK(glBindVertexArray(m_vao));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, buffer_size(vertices), NULL, GL_DYNAMIC_DRAW));

    GL_CHECK(glVertexAttribPointer(0, 2, GL_UNSIGNED_INT, GL_FALSE, sizeof(Vertex),
                                   (void *)offsetof(Vertex, coords)));

    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, buffer_size(indices), NULL, GL_DYNAMIC_DRAW));
    // DO NOT UNBIND EBO!

    GL_CHECK(glBindVertexArray(0)); // unbind vao.
}

void Polygon::set_data(span<Vertex> vertices, span<uint32_t> indices) {
    if (m_vbo == 0 && m_ebo == 0) {
        make_buffers(vertices, indices);
    }
    assert(m_vbo != 0);
    assert(m_ebo != 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, buffer_size(vertices), vertices.data()));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo));
    GL_CHECK(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, buffer_size(indices), indices.data()));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

    m_indices_uploaded = indices.size();
}

void Polygon::render_frame(const camera::Cam2d &cam, bool wireframe_mode) {
    if (m_indices_uploaded == 0)
        return;

    m_shader->attach();
    auto proj = cam.projection_maxtrix();
    GL_CHECK(glUniformMatrix4fv(glGetUniformLocation(m_shader->id, "proj"), 1, GL_FALSE,
                                glm::value_ptr(proj)));
    GL_CHECK(glUniform1f(glGetUniformLocation(m_shader->id, "scale"), (float)cam.zoom));

    GL_CHECK(glBindVertexArray(m_vao));

    if (!wireframe_mode)
        GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));

    GL_CHECK(glDrawElements(GL_TRIANGLES, m_indices_uploaded, GL_UNSIGNED_INT, 0));

    if (!wireframe_mode)
        GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    glBindVertexArray(0);
    m_shader->detach();
}
} // namespace polygon