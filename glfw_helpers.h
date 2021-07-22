#pragma once

namespace glfw_helpers {
// Class needed to be able to use closures as callbacks.
struct GLFWMouseController {
  // using Self = GLFWMouseController;
  using mouse_button_cb_t =
      std::function<void(GLFWwindow* wnd, int btn, int act, int mods)>;
  using mouse_scroll_cb_t =
      std::function<void(GLFWwindow* wnd, double xoffset, double yoffset)>;
  using mouse_move_cb_t =
      std::function<void(GLFWwindow* wnd, double xpos, double ypos)>;

  inline static mouse_button_cb_t btn_cb;
  inline static mouse_scroll_cb_t scroll_cb;
  inline static mouse_move_cb_t move_cb;

  static void set_mouse_button_callback(mouse_button_cb_t cb) {
    GLFWMouseController::btn_cb = std::move(cb);
  }
  static void set_mouse_scroll_callback(mouse_scroll_cb_t cb) {
    GLFWMouseController::scroll_cb = std::move(cb);
  }
  static void set_mouse_move_callback(mouse_move_cb_t cb) {
    GLFWMouseController::move_cb = std::move(cb);
  }

  static void mouse_move_callback(GLFWwindow* wnd, double xpos, double ypos) {
    if (move_cb) {
      move_cb(wnd, xpos, ypos);
    }
  }

  static void mouse_scroll_callback(GLFWwindow* wnd,
                                    double xoffset,
                                    double yoffset) {
    if (scroll_cb) {
      scroll_cb(wnd, xoffset, yoffset);
    }
  }

  static void mouse_button_callback(GLFWwindow* wnd,
                                    int btn,
                                    int act,
                                    int mods) {
    if (btn_cb) {
      btn_cb(wnd, btn, act, mods);
    }
  }
};
}  // namespace glfw_helpers