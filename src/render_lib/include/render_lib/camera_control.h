#pragma once

#include "camera.h"
#include <glm/glm.hpp>

namespace camera_control {

// Simple camera control for desktop applications supporting zooming around
// mouse center, rotation around desktop center and simple panning.
// todo: kinematic panning.
// todo: refactor to have just methods like (on_mouse_move(), on_mouse_click(),
// on_mouse_scroll). Basically this class maps user inputs *(mouse and keyboard)
// to camera control instructions (zoom/rotation/translations) it can even
// not be aware about camera class after all.
class CameraControl {
public: // todo: make it private once refactoring finished.
  camera::Cam2d &m_cam_ref;
  bool panning = false;
  int prev_x = 0, prev_y = 0; // use int to be able to compare with 0.
  bool rotation = false;
  double rotation_start =
      0.0; // camera rotation at the moment when rotation started.
  glm::vec2 rot_start_point, rot_curr_point, screen_center;

  camera::Cam2d &cam() { return m_cam_ref; }

public:
  CameraControl(camera::Cam2d &cam_ref) : m_cam_ref(cam_ref) {}

  void mouse_move(double xpos, double ypos) {
    if (this->panning) {
      if (this->prev_x == 0.0 && this->prev_y == 0.0) {
        this->prev_x = xpos;
        this->prev_y = ypos;
      }
      // todo: we can just save prev already unprojected.
      auto prev_u = cam().unproject(glm::vec2{this->prev_x, this->prev_y});
      auto curr_u = cam().unproject(glm::vec2{xpos, ypos});
      cam().focus_pos -= curr_u - prev_u;

      this->prev_x = xpos;
      this->prev_y = ypos;
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
    if (act == GLFW_PRESS && btn == GLFW_MOUSE_BUTTON_LEFT) {
      if (mods == GLFW_MOD_SHIFT) {
        double cx, cy;
        glfwGetCursorPos(wnd, &cx, &cy);
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
      }
    } else if (act == GLFW_RELEASE && btn == GLFW_MOUSE_BUTTON_LEFT) {
      if (this->rotation) {
        this->rotation = false;
        // todo: reset rest of the state just for hygyene purposes.
      } else if (this->panning) {
        this->panning = false;
        this->prev_x = 0;
        this->prev_y = 0;
      }
    }
  }

  // todo: get rid of glfw leak here.
  void mouse_scroll(GLFWwindow *wnd, double xoffset, double yoffset) {
    // We want to have scrolling around mouse position. In order
    // to achieve such effect we need to keep invariant that world
    // coordinates of mouse position not changed after zooming.
    double cx, cy;
    glfwGetCursorPos(wnd, &cx, &cy);
    auto mouse_pos_world = cam().unproject(glm::vec2{cx, cy});
    cam().zoom_pos = mouse_pos_world;
    cam().zoom =
        cam().zoom * pow(2, yoffset * 0.1); // zoom slightly more in than out.
    log_debug("camera-zoom: {}", cam().zoom);
    cam().zoom = std::clamp(cam().zoom, 0.000212537, 1000.0);
    auto mouse_pos_after_zoom_world = cam().unproject(glm::vec2{cx, cy});
    auto diff = mouse_pos_after_zoom_world - mouse_pos_world;
    cam().focus_pos -= diff;
    cam().zoom_pos = glm::vec2{cx, cy};

#ifndef NDEBUG
    auto control_diff = cam().unproject(glm::vec2{cx, cy}) - mouse_pos_world;
    //assert(control_diff[0] < 1.0 && control_diff[1] < 1.0);
    if (control_diff[0] > 1.0 || control_diff[1] > 1.0) {
      log_warn("control_diff[0] < 1.0 && control_diff[1] < 1.0");
    }    
#endif
  }
};

} // namespace camera_control