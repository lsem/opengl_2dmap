#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace camera {

struct Cam2d {
    glm::vec2 window_size;
    glm::vec2 focus_pos;
    double zoom = 3.0;
    double rotation = 0.0;
    glm::vec2 zoom_pos;

    // todo: this is view projection matrix.
    glm::mat4 projection_maxtrix() const {
        float w = this->window_size.x, h = this->window_size.y;
        auto projection_m = glm::ortho(0.0f, w, 0.0f, h);

        glm::mat4 view_m{1.0f};
        view_m = glm::translate(view_m, glm::vec3{w / 2.0f, h / 2.0f, 0.0f});              // 4-th
        view_m = glm::rotate(view_m, (float)this->rotation, glm::vec3{0.0f, 0.0f, -1.0f}); // 3-rd
        view_m = glm::scale(view_m, glm::vec3{this->zoom, this->zoom, 1.0});               // 2-nd
        view_m = glm::translate(view_m, glm::vec3{-focus_pos, 0.0f});                      // 1-st
        return projection_m * view_m;
    }

    glm::vec2 unproject(glm::vec2 p) const {
        double w = this->window_size.x, h = this->window_size.y;
        auto clip_p = glm::vec4{p[0] / w * 2.0 - 1.0,
                                (p[1] / h * 2.0 - 1.0) * -1.0, // *-1 because we have y flipped
                                                               // relative to opengl clip space
                                0.0, 1.0};
        return glm::inverse(projection_maxtrix()) * clip_p;
    }
};

} // namespace camera