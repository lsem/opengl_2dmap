#include "common/global.h"
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>

#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include "common/os.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

#include "common/log.h"
#include "gg/gg.h"
#include "glfw_helpers.h"
#include "render_lib/camera.h"
#include "render_lib/camera_control.h"
#include "render_lib/camera_control_visuals.h"
#include "render_units/crosshair/crosshair_unit.h"
#include "render_units/lines/lines_unit.h"
#include "render_units/roads/roads_unit.h"
#include "render_units/roads/tesselation.h"
#include "render_units/triangle/render_triangle.h"

#include "render_lib/shader_program.h"

using camera::Cam2d;
using camera_control::CameraControl;

using namespace std;

bool g_wireframe_mode = false;
bool g_prev_wireframe_mode = false;
bool g_show_crosshair = false;

unsigned camera_x, camera_y;
double camera_scale, camera_rotation;

void update_fps_counter(GLFWwindow *window) {
  static double previous_seconds = glfwGetTime();
  static int frame_count;
  double current_seconds = glfwGetTime();
  double elapsed_seconds = current_seconds - previous_seconds;
  if (elapsed_seconds > 0.25) {
    previous_seconds = current_seconds;
    double fps = (double)frame_count / elapsed_seconds;
    char tmp[128];
    sprintf(tmp, "opengl @ fps: %.2f", fps);
    glfwSetWindowTitle(window, tmp);
    frame_count = 0;
  }
  frame_count++;
}

void process_input(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    g_prev_wireframe_mode = g_wireframe_mode;
    g_wireframe_mode = true;
    g_show_crosshair = true;
  } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
    g_prev_wireframe_mode = g_wireframe_mode;
    g_wireframe_mode = false;
    g_show_crosshair = false;
  }
}

unsigned g_window_width, g_window_height;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  g_window_width = width;
  g_window_height = height;

  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}

int main() {

  const std::string SHADERS_ROOT = []() {
    if (std::getenv("SHADERS_ROOT")) {
      return std::string{std::getenv("SHADERS_ROOT")};
    }
    return std::string{};
  }();

  if (SHADERS_ROOT.empty()) {
    log_err("SHADERS_ROOT env variable not set");
    return -1;
  }

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  g_window_width = 800, g_window_height = 600;
  GLFWwindow *window = glfwCreateWindow(g_window_width, g_window_height,
                                        "Red Triangle", nullptr, nullptr);
  if (window == nullptr) {
    std::cerr << "Failed to open GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  camera::Cam2d cam;
  cam.window_size = glm::vec2{g_window_width, g_window_height};
  CameraControl cam_control{cam};

  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // ------------------------------------------------------------------------
  // Mouse control:
  //  press left mouse button and move left/right to pan window.
  //  press CTRL and use cursor to move around center.
  glfwSetCursorPosCallback(
      window, &glfw_helpers::GLFWMouseController::mouse_move_callback);
  glfwSetScrollCallback(
      window, &glfw_helpers::GLFWMouseController::mouse_scroll_callback);
  glfwSetMouseButtonCallback(
      window, &glfw_helpers::GLFWMouseController::mouse_button_callback);
  glfw_helpers::GLFWMouseController::set_mouse_move_callback(
      [&cam_control, &cam](auto *wnd, double xpos, double ypos) {
        cam_control.mouse_move(xpos, ypos);
      });
  glfw_helpers::GLFWMouseController::set_mouse_button_callback(
      [&cam_control, &cam](GLFWwindow *wnd, int btn, int act, int mods) {
        cam_control.mouse_click(wnd, btn, act, mods);
      });
  glfw_helpers::GLFWMouseController::set_mouse_scroll_callback(
      [&cam, &cam_control](GLFWwindow *wnd, double xoffset, double yoffset) {
        cam_control.mouse_scroll(wnd, xoffset, yoffset);
      });
  // ------------------------------------------------------------------------

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glfwSwapInterval(0); // vsync
  glfwShowWindow(window);

  CrosshairUnit crosshair;
  if (!crosshair.load_shaders(SHADERS_ROOT)) {
    log_err("failed loading crosshair shaders");
    return -1;
  }

  TriangleUnit triangle;
  if (!triangle.load_shaders(SHADERS_ROOT)) {
    log_err("failed loading triangle shaders");
    return -1;
  }
  triangle.upload_geometry();

  CameraControlVisuals cam_control_vis;
  if (!cam_control_vis.init_render(SHADERS_ROOT)) {
    glfwTerminate();
    return -1;
  }

  cam.focus_pos = triangle.triangle_center();
  cam.zoom = 0.1;

  DebugCtx dctx;

  vector<p32> triangles_output;

  {
    v2 origin = v2{10000.0, 20000.0} + v2{200.0, 200.0};
    auto p1 = from_v2(origin + v2{0.0, 0.0});
    auto p2 = from_v2(origin + v2{100.0, 100.0} * 100);
    auto p3 = from_v2(origin + v2{200.0, 80.0} * 100);
    auto p4 = from_v2(origin + v2{300.0, 200.0} * 100);
    auto p5 = from_v2(p4 + v2{350.0, 100.0} * 100);
    auto p6 = from_v2(p5 + v2{400.0, 100.0} * 100);
    auto p7 = from_v2(p6 + v2{-400.0, +100.0} * 100);
    auto p8 = from_v2(p7 + v2{0.0, -300.0} * 100);
    vector<p32> input{{p1, p2, p3, p4, p5, p6, p7, p8}};
    // vector<p32> input;
    // for (int i = 0; i < 300; ++i) {
    //   auto rand_p = cam.unproject(glm::vec2{rand() % 800, rand() % 600});
    //   p32 r{static_cast<uint32_t>(rand_p.x),
    //   static_cast<uint32_t>(rand_p.y)}; input.push_back(r);
    //   //      std::cout << cam.unproject(ys) << "\n";
    // }
    // triangles_output.resize(
    //     (input.size() - 1) *
    //     8); // input.size() - 1 segments, 8 points for each for both sides
    //         // (could be reduced to 6 in fact)

    // todo: we can ask tesselator to give us this number depending on settings.
    triangles_output.resize((input.size() - 1) *
                            12); // if we used ebo, we could have 4 points per
                                 // quad instead of 6. but this requires quite
                                 // alot of indices, so I doubt this would help.

    vector<p32> outline_output(input.size() * 2);
    vector<p32> aa_outline_output(input.size() * 2);
    roads::tesselation::generate_geometry<
        roads::tesselation::FirstPassSettings>(input, triangles_output,
                                               outline_output, dctx);

    std::vector<p32> aa_triangles_output{outline_output.size() * 2 * 3 * 3};
    roads::tesselation::generate_geometry<roads::tesselation::AAPassSettings>(
        outline_output, aa_triangles_output, aa_outline_output, dctx);

    vector<Color> line_colors = {colors::red /*,colors::green,colors::blue*/};
    int curr_col = 0;
    p32 prev_p;
    bool first = true;
    for (auto p : outline_output) {
      if (!first) {
        // std::cout << "line: " << prev_p << ", " << p << "\n";
        // dctx.add_line(prev_p, p, line_colors[curr_col++ %
        // line_colors.size()]);
      }
      prev_p = p;
      first = false;
    }

  // to show triangles.
    for (size_t i = 3; i < triangles_output.size(); i += 4) {
      auto p1 = triangles_output[i - 3];
      auto p2 = triangles_output[i - 2];
      auto p3 = triangles_output[i - 1];
      auto p4 = triangles_output[i - 0];
      // dctx.add_line(p1, p2, colors::magenta); // p1 - p2 - p3
      // dctx.add_line(p2, p3, colors::magenta);
      // dctx.add_line(p3, p1, colors::magenta);
      // dctx.add_line(p1, p4, colors::magenta);
      // dctx.add_line(p4, p3, colors::magenta);
      // dctx.add_line(p3, p1, colors::magenta);
    }
  }
  LinesUnit road_dbg_lines{(unsigned)dctx.lines.size()};
  if (!road_dbg_lines.load_shaders(SHADERS_ROOT)) {
    std::cerr << "failed initializing renderer for road_dbg_lines\n";
    glfwTerminate();
    return -1;
  }

  vector<tuple<v2, v2, Color>> lines_v2;
  for (auto [a, b, c] : dctx.lines) {
    lines_v2.push_back(tuple{v2{a}, v2{b}, c});
  }
  road_dbg_lines.assign_lines(lines_v2);

  RoadsUnit roads;
  if (!roads.load_shaders(SHADERS_ROOT)) {
    log_err("failed loading shaders for roads");
    return -1;
  }
  if (!roads.make_buffers()) {
    log_err("failed creating buffers for roads");
    return -1;
  }
  roads.set_data(triangles_output);

  while (!glfwWindowShouldClose(window)) {
    update_fps_counter(window);
    process_input(window);

    glClearColor(.9f, .9f, .9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (g_wireframe_mode != g_prev_wireframe_mode) {
      if (g_wireframe_mode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      }
    };

    cam_control_vis.render(cam, cam_control);
    road_dbg_lines.render_frame(cam);
    triangle.render_frame(cam);
    roads.render_frame(cam);
    if (g_show_crosshair) {
      crosshair.render_frame(cam);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
    // in case window size has changed, make camera aware of it.
    // this should have been done in framebuffer callback but I cannot
    // capture camera into plain C function.
    cam.window_size = glm::vec2{g_window_width, g_window_height};
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
