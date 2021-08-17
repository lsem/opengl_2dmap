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

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

using camera::Cam2d;
using camera_control::CameraControl;

using namespace std;

bool g_wireframe_mode = false;
bool g_prev_wireframe_mode = false;
bool g_show_crosshair = false;

uint32_t MASTER_ORIGIN_X = 100'000'00;
uint32_t MASTER_ORIGIN_Y = 100'000'00;

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

std::tuple<vector<p32>, vector<ColoredVertex>, DebugCtx>
generate_random_roads(float cam_zoom) {
  DebugCtx dctx;

  //
  // Generate random polylines
  //
  const double k = 100.0;

  vector<p32> all_roads_triangles(100'000'00); // todo: calculate size correctly
  vector<p32> all_roads_aa_triangles(100'000'00);
  size_t current_offset = 0;
  size_t current_aa_offset = 0;

  srand(clock());

  std::chrono::steady_clock::duration tesselation_time;

  for (int i = 0; i < 500; ++i) {
    int random_segments_count = rand() % 30 + 30;
    double random_vector_angle = ((rand() % 360) / 360.0) * 2 * M_PI;
    const int scater = 60000;
    v2 origin =
        v2{MASTER_ORIGIN_X, MASTER_ORIGIN_Y} +
        (v2{rand() % scater - (scater / 2), rand() % scater - (scater / 2)});
    v2 prev_p = origin;

    vector<p32> random_polyline = {from_v2(prev_p)};
    for (int s = 0; s < random_segments_count; s++) {
      double random_length = (double)(rand() % 100000 + 10000);
      double random_angle_val = (rand() % 5 + 5);
      if (rand() % 2 == 1) {
        random_angle_val *= -1;
      }
      double random_angle =
          random_vector_angle + (random_angle_val / 360.0) * 2 * M_PI;
      auto random_direction =
          normalized(v2(std::cos(random_angle), std::sin(random_angle)));
      random_vector_angle = random_angle;
      // log_debug("random_length: {}", random_length);
      // log_debug("random_angle: {} ({})", random_angle,
      //(random_angle / M_PI) * 180.0);

      v2 p = prev_p + random_direction * random_length;

      if (p.x < 0.0 || p.y < 0.0) {
        log_warn("out of canvas. truncate road");
        break;
      }
      // dctx.add_line(prev_p, p, colors::grey);
      prev_p = p;

      random_polyline.push_back(from_v2(p));
    }
    if (random_polyline.size() < 3) {
      log_warn("two small road, skip");
      continue;
    }

    const size_t one_road_triangles_vertex_count =
        (random_polyline.size() - 1) * 12;
    span<p32> random_road_triangles_span(all_roads_triangles.data() +
                                             current_offset,
                                         one_road_triangles_vertex_count);

    current_offset += one_road_triangles_vertex_count;

    vector<p32> outline_output(
        random_polyline.size() *
        2); // outline must be have two lines per one segment.

    auto tesselation_start_time = std::chrono::steady_clock::now();

    roads::tesselation::generate_geometry<
        roads::tesselation::FirstPassSettings>(random_polyline,
                                               random_road_triangles_span,
                                               outline_output, 2000.0, dctx);

    const size_t one_road_aa_triangles_vertex_count =
        (outline_output.size() - 1) * 12;
    span<p32> random_road_aa_triangles_span(all_roads_aa_triangles.data() +
                                                current_aa_offset,
                                            one_road_aa_triangles_vertex_count);
    current_aa_offset += one_road_aa_triangles_vertex_count;

    vector<p32> no_outline_for_aa_pass; // just fake placeholder.
    roads::tesselation::generate_geometry<roads::tesselation::AAPassSettings>(
        outline_output, random_road_aa_triangles_span, no_outline_for_aa_pass,
        5.0 / cam_zoom, dctx); // in world coordiantes try smth like 200.0

    tesselation_time +=
        (std::chrono::steady_clock::now() - tesselation_start_time);
  }

  auto tesselation_time_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(tesselation_time)
          .count();
  log_debug("Tesselation time: {}ms", tesselation_time_ms);

  // For now, vertices that produced by tesselation function have different
  // format from what roads renderer expects, so we need to to this
  // artificial mapping.
  // this transformations is artificial and temporary, we can get rid of that by
  // abstracing out output container from  algorithm so that it can accept not
  // only span<p32> but span<ColoredVertex>.
  vector<ColoredVertex> aa_data(current_aa_offset);
  const auto COLOR = Color{0.53, 0.54, 0.55, 1.0};
  const auto NOCOLOR = Color{1.0f, 1.0f, 1.0f, 1.0f}; // color of glClearCOLOR
  const vector<Color> AA_VERTEX_COLORS_PATTERN = {COLOR,   NOCOLOR, NOCOLOR,
                                                  NOCOLOR, COLOR,   COLOR};
  std::transform(
      std::begin(all_roads_aa_triangles),
      std::begin(all_roads_aa_triangles) + current_aa_offset,
      std::begin(aa_data), [i = 0, &AA_VERTEX_COLORS_PATTERN](p32 x) mutable {
        return ColoredVertex(
            x, AA_VERTEX_COLORS_PATTERN[i++ % AA_VERTEX_COLORS_PATTERN.size()]);
      });

  all_roads_triangles.resize(current_offset);
  all_roads_aa_triangles.resize(current_aa_offset);

  return std::tuple{all_roads_triangles, aa_data, dctx};
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

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(NULL);
  // Setup Dear ImGui style
  ImGui::StyleColorsLight();

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

  // cam.focus_pos = triangle.triangle_center();
  cam.focus_pos = glm::vec2{MASTER_ORIGIN_X, MASTER_ORIGIN_Y};
  cam.zoom = 0.1;

  RoadsUnit roads;
  if (!roads.load_shaders(SHADERS_ROOT)) {
    log_err("failed loading shaders for roads");
    return -1;
  }
  if (!roads.make_buffers()) {
    log_err("failed creating buffers for roads");
    return -1;
  }

  auto [roads_data, roads_aa_data, dctx] = generate_random_roads(cam.zoom);
  roads.set_data(roads_data, roads_aa_data);

  // Debug context lines.
  vector<tuple<v2, v2, Color>> lines_v2;
  for (auto [a, b, c] : dctx.lines) {
    lines_v2.push_back(tuple{v2{a}, v2{b}, c});
  }
  LinesUnit road_dbg_lines{(unsigned)dctx.lines.size()};
  if (!road_dbg_lines.load_shaders(SHADERS_ROOT)) {
    std::cerr << "failed initializing renderer for road_dbg_lines\n";
    glfwTerminate();
    return -1;
  }
  road_dbg_lines.assign_lines(lines_v2);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    update_fps_counter(window);
    process_input(window);

    // feed inputs to dear imgui, start new frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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

    ImGui::Text("Hello, world %d", 123);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    // in case window size has changed, make camera aware of it.
    // this should have been done in framebuffer callback but I cannot
    // capture camera into plain C function.
    cam.window_size = glm::vec2{g_window_width, g_window_height};
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
