#pragma once

#include "animations.h"
#include "camera.h"
#include <common/log.h>

#include <glm/glm.hpp>
#include <imgui/imgui.h>

namespace camera_control {

// Simple camera control for desktop applications supporting zooming around
// mouse center, rotation around desktop center and simple panning.
// todo: kinematic panning.
// todo: refactor to have just methods like (on_mouse_move(), on_mouse_click(),
// on_mouse_scroll). Basically this class maps user inputs *(mouse and keyboard)
// to camera control instructions (zoom/rotation/translations) it can even
// not be aware about camera class after all.
class CameraControl {
  public:
    camera::Cam2d &m_cam_ref;
    bool panning = false;
    bool rotation = false;
    double rotation_start = 0.0; // camera rotation at the moment when rotation started.
    glm::vec2 rot_start_point, rot_curr_point, screen_center;
    animations::AnimationsEngine &m_animations_engine;
    animations::easing_func_t m_easing_func = animations::easing_funcs::ease_out_quad;
    int animation_speed_ms = 300;
    steady_clock::time_point m_last_move_event_tp;
    optional<v2> m_maybe_last_pos;
    v2 m_current_velocity;
    double m_current_velocity_magnitude;
    bool m_animations_progress = false;
    bool m_kinetic_scrolling_enabled = true;

    camera::Cam2d &cam() { return m_cam_ref; }

  public:
    CameraControl(camera::Cam2d &cam_ref, animations::AnimationsEngine &animations_engine)
        : m_cam_ref(cam_ref), m_animations_engine(animations_engine) {}

    void process_animations(steady_clock::time_point tp = steady_clock::now()) {
        if (!m_animations_progress) {
            return;
        }

        m_current_velocity_magnitude *= 0.97;

        auto dt = tp - m_last_move_event_tp;
        auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dt).count();
        if (m_current_velocity_magnitude > 0.000001) {
            v2 screen_displacement = -normalized(m_current_velocity) *
                                     m_current_velocity_magnitude *
                                     (double)dt_ms; // displacement in pixels per milliseconds

            cam().focus_pos = cam().unproject(cam().screen_center() + screen_displacement);
            m_last_move_event_tp = tp;
        } else {
            m_animations_progress = false;
        }
    }

    void mouse_move(double xpos, double ypos) {
        auto curr_pos = v2(xpos, ypos);
        auto move_event_tp = steady_clock::now();

        if (this->panning) {
            auto &last_pos = *m_maybe_last_pos;

            auto dt = move_event_tp - m_last_move_event_tp;
            auto df = curr_pos - last_pos;

            m_current_velocity =
                df / std::chrono::duration_cast<std::chrono::milliseconds>(dt).count();

            cam().focus_pos -= (cam().unproject(curr_pos) - cam().unproject(last_pos));

            m_maybe_last_pos = curr_pos;
            m_last_move_event_tp = move_event_tp;

        } else if (this->rotation) {
            this->rot_curr_point = glm::vec2{xpos, ypos};
            // angle between these start and current vectors is our rotation.
            auto A = glm::normalize(this->rot_start_point - this->screen_center);
            auto B = glm::normalize(this->rot_curr_point - this->screen_center);
            // todo: (separate commit) why I'm not using atan2?:
            // atan2(A[1], A[0]) - atan2(B[1], B[0]);
            auto angle = std::acos(glm::dot(glm::normalize(A), glm::normalize(B)));
            auto Vn = glm::vec3{0.0f, 0.0f, 1.0f};
            auto crossAB = glm::cross(glm::vec3{A, 0.0}, glm::vec3{B, 0.0});
            auto signV = glm::dot(Vn, crossAB);
            if (signV < 0) {
                angle *= -1;
            }
            cam().rotation = this->rotation_start + angle;
        }
    }

    // todo: get rid of glfw leak here.
    void mouse_click(GLFWwindow *wnd, int btn, int act, int mods) {
        m_animations_progress = false;

        if (act == GLFW_PRESS && btn == GLFW_MOUSE_BUTTON_LEFT) {
            double cx, cy;
            glfwGetCursorPos(wnd, &cx, &cy);

            if (mods == GLFW_MOD_SHIFT) {
                this->rotation = true;
                this->rotation_start = cam().rotation;
                // we want to have this vectors used for calculation rotation in
                // screen space because after changing rotation they would be
                // invalidated.
                this->rot_start_point = glm::vec2{cx, cy};
                this->rot_curr_point = this->rot_start_point;
                this->screen_center = cam().window_size / 2.0f; // pivot point?
            } else {
                // todo: start position can be captured here
                this->panning = true;
                m_maybe_last_pos = v2(cx, cy);
                m_last_move_event_tp = steady_clock::now();
            }
        } else if (act == GLFW_RELEASE && btn == GLFW_MOUSE_BUTTON_LEFT) {
            if (this->rotation) {
                this->rotation = false;
                // todo: reset rest of the state just for hygyene purposes.
            } else if (this->panning) {
                this->panning = false;
                m_maybe_last_pos = std::nullopt;

                // we are going to continue scrolling based on current moment velocity
                // so lets initialize the process.
                if (m_kinetic_scrolling_enabled) {
                    m_current_velocity_magnitude = len(m_current_velocity);
                    if (m_current_velocity_magnitude > 0.000001 &&
                        !std::isinf(m_current_velocity_magnitude)) {
                        m_last_move_event_tp = steady_clock::now();
                        m_animations_progress = true;
                    } else {
                        log_debug(
                            "kinetic scroll not started, magnitude is lower than threshold: {}",
                            m_current_velocity_magnitude);
                    }
                }
            }
        }
    }

    // todo: get rid of glfw leak here.
    void mouse_scroll(GLFWwindow *wnd, double xoffset, double yoffset) {
        // We want to have scrolling around mouse position. In order
        // to achieve such effect we need to keep invariant that world
        // coordinates of mouse position not changed after zooming.

        auto &camera = cam();

        auto *animation = m_animations_engine.find_animation(&camera.zoom);
        const double target_zoom =
            (animation ? animation->target_value : camera.zoom) * pow(2, yoffset * 0.2);

        double cx, cy;
        glfwGetCursorPos(wnd, &cx, &cy);
        auto screen_zoom_center = glm::vec2(cx, cy);
        auto prev_world_zoom_center = camera.unproject(screen_zoom_center);

        m_animations_engine.animate(
            &camera.zoom, target_zoom, std::chrono::milliseconds(animation_speed_ms), m_easing_func,
            [&camera, screen_zoom_center, prev_world_zoom_center]() mutable {
                auto world_zoom_center = camera.unproject(screen_zoom_center);
                camera.focus_pos -= (world_zoom_center - prev_world_zoom_center);
                prev_world_zoom_center = camera.unproject(screen_zoom_center);
            },
            [] { /* finish */ });
    }

    void render_gui() {
        if (ImGui::CollapsingHeader("Camera Control")) {
            ImGui::Checkbox("Kinetic Scrolling", &m_kinetic_scrolling_enabled);

            ImGui::SliderInt("Animation\nSpeed", &animation_speed_ms, 0, 500, NULL,
                             ImGuiSliderFlags_AlwaysClamp);

            ImGui::BeginListBox("Camera\nAnimation\nEasing");

            if (ImGui::Selectable("linear", m_easing_func == animations::easing_funcs::linear)) {
                m_easing_func = animations::easing_funcs::linear;
            }
            if (ImGui::Selectable("ease_out_quad",
                                  m_easing_func == animations::easing_funcs::ease_out_quad)) {
                m_easing_func = animations::easing_funcs::ease_out_quad;
            }
            if (ImGui::Selectable("ease_in_out_quad",
                                  m_easing_func == animations::easing_funcs::ease_in_out_quad)) {
                m_easing_func = animations::easing_funcs::ease_in_out_quad;
            }
            ImGui::EndListBox();
        }
    }

}; // namespace camera_control

} // namespace camera_control