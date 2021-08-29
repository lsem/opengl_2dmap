#include "common/global.h"
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>

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
#include "render_units/lands/lands.h"
#include "render_units/lines/lines_unit.h"
#include "render_units/roads/roads_unit.h"
#include "render_units/roads/tesselation.h"
#include "render_units/roads_shader_aa/make_geometry.h"
#include "render_units/roads_shader_aa/roads_shader_aa_unit.h"
#include "render_units/triangle/render_triangle.h"
#include <type_traits>

#include "render_lib/debug_ctx.h"
#include "render_lib/shader_program.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <mapbox/earcut.hpp>

#include <map_compiler_lib.h>
#include <render_lib/animations.h>

using camera::Cam2d;
using camera_control::CameraControl;

using namespace std;

bool g_wireframe_mode = false;
bool g_prev_wireframe_mode = false;
bool g_show_crosshair = false;

// uint32_t MASTER_ORIGIN_X = gg::U32_MAX / 2;
// uint32_t MASTER_ORIGIN_Y = gg::U32_MAX / 2;

uint32_t MASTER_ORIGIN_X = gg::lon_to_x(23.0);
uint32_t MASTER_ORIGIN_Y = gg::lat_to_y(49.0);

const v2 RANDOM_ROADS_SCENE_POSITION(MASTER_ORIGIN_X, MASTER_ORIGIN_Y);

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
  // and height will be significantly larger than specified on retina
  // displays.
  glViewport(0, 0, width, height);
}

void print_coords_debug_info(const Cam2d &cam, double cx, double cy) {
  glm::vec2 q = cam.unproject(glm::vec2{cx, cy});
  auto x = static_cast<uint32_t>(q.x);
  auto y = static_cast<uint32_t>(q.y);
  double lat = gg::mercator::yu_to_lat(y);
  double lon = gg::mercator::xu_to_lon(x);
  log_debug("Click: {},{} (x: {}, y:{})", lat, lon, x, y);
}

// Returns {vertices, indices, aa_vertices, aa_indices} for lands.
std::optional<std::tuple<vector<p32>, vector<uint32_t>,
                         vector<roads_shader_aa::AAVertex>, vector<uint32_t>>>
generate_lands_quads(std::string data_root_str, DebugCtx &dctx) {
  vector<p32> vertices;
  vector<uint32_t> indices;
  vector<roads_shader_aa::AAVertex> aa_vertices(50'000'000);
  vector<uint32_t> aa_indices(50'000'000);
  size_t current_aa_vertices_offset = 0;
  size_t current_aa_indiices_offset = 0;

  std::chrono::steady_clock::duration total_earcut_time{0};
  std::chrono::steady_clock::duration total_aa_time{0};

  try {
    auto lands_path = fs::path(data_root_str) / "natural_earth" /
                      "ne_10m_land" / "ne_10m_land.shp";

    for (auto &shape : map_compiler::load_shapes(lands_path)) {
      int parn_n = 0;
      for (auto &part : shape) {
        assert(part.front() == part.back());

        // Convert part vertices into mapbox::earcut format which
        // expects polygin defined as vector of vectors which means
        // main polygon and holes.
        auto start_time = std::chrono::steady_clock::now();
        vector<vector<array<double, 2>>> earcut_polygon;
        earcut_polygon.push_back({});
        bool first = true;
        const size_t M = vertices.size();
        for (gg::gpt_t &pt : part) {
          earcut_polygon.back().push_back(
              {static_cast<double>(pt.x), static_cast<double>(pt.y)});
          vertices.emplace_back(pt);
        }
        for (auto idx : mapbox::earcut(earcut_polygon)) {
          indices.push_back(idx + M);
        }
        total_earcut_time += std::chrono::steady_clock::now() - start_time;

        auto aa_start_time = std::chrono::steady_clock::now();
        assert(indices.size() % 3 == 0);

        { // debug
          auto pen = dctx.make_pen();
          auto it = std::begin(vertices) + M;
          pen.move_to(*it++);
          int k = 0;
          for (; it != std::end(vertices); ++it) {
            vector<Color> colors({colors::red, colors::green, colors::blue});
            auto c = colors[parn_n];
            c.r *= k / (double)M;
            c.g *= k / (double)M;
            c.b *= k / (double)M;
            pen.line_to(*it, c);
            k++;
          }
        }
        auto [aa_vertices_generated, aa_indices_generated] =
            roads_shader_aa::make_geometry(
                span(std::begin(vertices) + M, std::end(vertices)), 1.0,
                aa_vertices, current_aa_vertices_offset, aa_indices,
                current_aa_indiices_offset, dctx);

        { // debug
          for (size_t i = current_aa_indiices_offset + 2;
               i < current_aa_indiices_offset + aa_indices_generated; ++i) {
            auto a = aa_vertices[aa_indices[i - 2]].coords;
            auto b = aa_vertices[aa_indices[i - 1]].coords;
            auto c = aa_vertices[aa_indices[i]].coords;

            vector<Color> colors({colors::red, colors::green, colors::blue});
            // dctx.add_line(a, b, colors[parn_n % colors.size()]);
            // dctx.add_line(b, c, colors[parn_n % colors.size()]);
            // dctx.add_line(c, a, colors[parn_n % colors.size()]);

            auto a_u =
                std::make_tuple(gg::mercator::unproject_lon(gg::x_to_lon(a.x)),
                                gg::mercator::unproject_lat(gg::y_to_lat(a.y)));

            auto b_u =
                std::make_tuple(gg::mercator::unproject_lon(gg::x_to_lon(b.x)),
                                gg::mercator::unproject_lat(gg::y_to_lat(b.y)));

            auto c_u =
                std::make_tuple(gg::mercator::unproject_lon(gg::x_to_lon(c.x)),
                                gg::mercator::unproject_lat(gg::y_to_lat(c.y)));
          }
        } // debug scope

        current_aa_vertices_offset += aa_vertices_generated;
        current_aa_indiices_offset += aa_indices_generated;

        total_aa_time += std::chrono::steady_clock::now() - aa_start_time;
        parn_n++;
      } // parts

    } // shapes
  } catch (const std::exception &e) {
    log_err("failed compiling lands: {}", e.what());
    return std::nullopt;
  }
  log_debug(
      "Lands Triangulation time: {}ms",
      std::chrono::duration_cast<std::chrono::milliseconds>(total_earcut_time)
          .count());
  log_debug("Lands AA time: {}ms",
            std::chrono::duration_cast<std::chrono::milliseconds>(total_aa_time)
                .count());

  if (current_aa_vertices_offset == aa_vertices.size()) {
    log_warn("Lends: aa vertices truncated");
  }
  if (current_aa_indiices_offset == aa_indices.size()) {
    log_warn("Lands: aa indices truncated");
  }
  aa_vertices.resize(current_aa_vertices_offset);
  aa_indices.resize(current_aa_indiices_offset);

  for (auto &v : aa_vertices) {
    v.color[0] = 0.53;
    v.color[1] = 0.54;
    v.color[2] = 0.55;
  }

  return std::tuple{std::move(vertices), std::move(indices),
                    std::move(aa_vertices), std::move(aa_indices)};
}

// Trying to reproduce a bug found during AA'ing lands.
std::tuple<vector<roads_shader_aa::AAVertex>, vector<uint32_t>>
generate_bug_scene() {
  vector<p32> points({p32(1596476416, 2683028992), p32(1699909504, 2703048448),
                      p32(1777391488, 2666346240), p32(1789254784, 2586639872),
                      p32(1867107584, 2551791616),
                      p32(1877488000, 2506192128)});

  vector<p32> shifted_points = points;
  std::for_each(shifted_points.begin(), shifted_points.end(), [](auto &p) {
    p.x += gg::U32_MAX / 24;
    p.y += gg::U32_MAX / 24;
  });

  vector<roads_shader_aa::AAVertex> vertices(1'000'000);
  vector<uint32_t> indices(1'000'000);

  DebugCtx ctx;
  size_t vertices_offset = 0;
  size_t indicies_offset = 0;
  auto [verices_num1, indices_num1] = roads_shader_aa::make_geometry(
      points, 15, vertices, vertices_offset, indices, indicies_offset, ctx);
  vertices_offset += verices_num1;
  indicies_offset += indices_num1;

  auto [verices_num2, indices_num2] = roads_shader_aa::make_geometry(
      shifted_points, 15, vertices, vertices_offset, indices, indicies_offset,
      ctx);

  vertices_offset += verices_num2;
  indicies_offset += indices_num2;

  vertices.resize(vertices_offset);
  indices.resize(indicies_offset);

  for (auto &v : vertices) {
    v.color[0] = 0.83;
    v.color[1] = 0.54;
    v.color[2] = 0.55;
  }

  return tuple{std::move(vertices), std::move(indices)};
}

std::tuple<vector<roads_shader_aa::AAVertex>, vector<uint32_t>, vector<p32>,
           DebugCtx>
generate_test_scene2(v2 scene_origin, float zoom) {
  vector<p32> all_roads_triangles(10'000'000); // todo: calculate size correctly
  vector<roads_shader_aa::AAVertex> all_roads_aa_triangles(10'000'000);

  double scale = 1000;
  auto p1 = scene_origin;
  auto p2 = p1 + v2(10, 10) * scale;
  auto p3 = p2 + v2(10, -20) * scale;
  auto p4 = p3 + v2(0, 20) * scale;
  vector<p32> road_one{from_v2(p1), from_v2(p2), from_v2(p3), from_v2(p4)};

  const size_t vertex_count = (road_one.size() - 1) * 12;
  span<p32> road_one_span(all_roads_triangles.data(), vertex_count);
  vector<p32> outline(road_one.size() * 2);

  DebugCtx ctx;

  roads::tesselation::generate_geometry<roads::tesselation::FirstPassSettings>(
      road_one, road_one_span, outline, 2000.0, ctx);
  all_roads_triangles.resize(vertex_count);

  vector<roads_shader_aa::AAVertex> all_vertices(1000'000);
  vector<uint32_t> all_indices(1000'000);

  auto [verices_num, indices_num] = roads_shader_aa::make_geometry(
      outline, 1.5, all_vertices, 0, all_indices, 0, ctx);
  all_vertices.resize(verices_num);
  all_indices.resize(indices_num);
  for (auto &v : all_vertices) {
    v.color[0] = 0.83;
    v.color[1] = 0.54;
    v.color[2] = 0.55;
  }

  log_debug("vertices number: {}", verices_num);
  log_debug("indices number: {}", indices_num);

  //  Do what opengl supposed to be in indexed drawing mode.
  // for (size_t i = 2; i < all_indices.size(); i += 3) {
  //   ctx.add_line(all_vertices[all_indices[i - 2]],
  //                all_vertices[all_indices[i - 1]], colors::grey);
  //   ctx.add_line(all_vertices[all_indices[i - 1]],
  //   all_vertices[all_indices[i]],
  //                colors::grey);
  //   ctx.add_line(all_vertices[all_indices[i]], all_vertices[all_indices[i -
  //   2]],
  //                colors::grey);
  // }

  return tuple{std::move(all_vertices), std::move(all_indices),
               std::move(all_roads_triangles), std::move(ctx)};
}

std::tuple<vector<p32>, vector<ColoredVertex>, DebugCtx>
generate_test_scene(v2 scene_origin) {

  vector<p32> all_roads_triangles(10'000'000); // todo: calculate size correctly
  vector<p32> all_roads_aa_triangles(10'000'000);

  double scale = 1000;
  auto p1 = scene_origin;
  auto p2 = p1 + v2(10, 10) * scale;
  auto p3 = p2 + v2(10, -20) * scale;
  auto p4 = p3 + v2(0, 20) * scale;
  vector<p32> road_one{from_v2(p1), from_v2(p2), from_v2(p3), from_v2(p4)};

  const size_t vertex_count = (road_one.size() - 1) * 12;
  span<p32> road_one_span(all_roads_triangles.data(), vertex_count);
  vector<p32> outline(road_one.size() * 2);

  DebugCtx ctx;

  roads::tesselation::generate_geometry<roads::tesselation::FirstPassSettings>(
      road_one, road_one_span, outline, 2000.0, ctx);

  const size_t aa_vertex_count = (outline.size() - 1) * 12;
  span<p32> aa_data_span(all_roads_aa_triangles.data(), aa_vertex_count);

  vector<p32> no_outline_for_aa_pass; // just fake placeholder.
  roads::tesselation::generate_geometry<roads::tesselation::AAPassSettings>(
      outline, aa_data_span, no_outline_for_aa_pass, 100,
      ctx); // in world coordiantes try smth like 200.0

  // Given a road polyline, we need to generate vertices and
  //  indicies.
  // for aa we need to produce two vertices for each point of outline and
  // indicices that makes vertices. then this indicies will produce vertices,
  // and we need to pass vector V along with that vertex.
  // this information will be used by shader for calculating.
  // Given outline I want to generate

  // I already have outline, what I need is to produce two indices for the
  // same points of outline and specify vector for that point. then, so I will
  // have two vertices with same information, how am I supposed to figure out
  // which one is internal and which one is external? one approach is to use
  // vertex id. but what if that does not scale? are there any other ways?

  // for each point of outline, we produce two vertices v1, v2.
  // then we make two triangles with previous vetices v1', v2':
  //  indices.push_back(index(v1'))
  //  indices.push_back(index(v2))
  //  indices.push_back(index(v1))
  //  indices.push_back(index(v2'))
  //  indices.push_back(index(v2))
  //  indices.push_back(index(v1))

  all_roads_triangles.resize(vertex_count);
  all_roads_aa_triangles.resize(aa_vertex_count);

  vector<ColoredVertex> aa_data(aa_vertex_count);
  const auto COLOR = Color{0.53, 0.54, 0.55, 1.0};
  const auto NOCOLOR = Color{1.0f, 1.0f, 1.0f, 1.0f}; // color of glClearCOLOR
  const vector<Color> AA_VERTEX_COLORS_PATTERN = {COLOR,   NOCOLOR, NOCOLOR,
                                                  NOCOLOR, COLOR,   COLOR};
  std::transform(
      std::begin(all_roads_aa_triangles), std::end(all_roads_aa_triangles),
      std::begin(aa_data), [i = 0, &AA_VERTEX_COLORS_PATTERN](p32 x) mutable {
        return ColoredVertex(
            x, AA_VERTEX_COLORS_PATTERN[i++ % AA_VERTEX_COLORS_PATTERN.size()]);
      });

  return std::tuple{all_roads_triangles, aa_data, ctx};
}

std::tuple<vector<p32>, DebugCtx> generate_random_roads(v2 scene_origin,
                                                        float cam_zoom) {
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
    v2 origin = scene_origin + (v2(rand() % scater - (scater / 2),
                                   rand() % scater - (scater / 2)));
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

    tesselation_time +=
        (std::chrono::steady_clock::now() - tesselation_start_time);
  }

  auto tesselation_time_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(tesselation_time)
          .count();
  log_debug("Tesselation time: {}ms", tesselation_time_ms);

  all_roads_triangles.resize(current_offset);
  return std::tuple{all_roads_triangles, dctx};
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
        if (btn == GLFW_MOUSE_BUTTON_LEFT && act == GLFW_PRESS) {
          double cx, cy;
          glfwGetCursorPos(wnd, &cx, &cy);
          print_coords_debug_info(cam, cx, cy);
        }
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
  cam.zoom = 7.227499802162154e-07;

  //
  // Roads
  //
  RoadsUnit roads;
  if (!roads.load_shaders(SHADERS_ROOT)) {
    log_err("failed loading shaders for roads");
    return -1;
  }
  if (!roads.make_buffers()) {
    log_err("failed creating buffers for roads");
    return -1;
  }
  roads_shader_aa::RoadsShaderAAUnit roads_shaders_aa;
  if (!roads_shaders_aa.load_shaders(SHADERS_ROOT)) {
    log_err("failed loading shaders for roads_shaders_aa");
    return -1;
  }
  if (!roads_shaders_aa.make_buffers()) {
    log_err("failed creating buffers roads_shaders_aa");
    return -1;
  }
  auto [roads_data, dctx] =
      generate_random_roads(RANDOM_ROADS_SCENE_POSITION, cam.zoom);
  roads.set_data(roads_data);

  //
  // Debug Scene
  //
  roads_shader_aa::RoadsShaderAAUnit debug_scene;
  if (!debug_scene.load_shaders(SHADERS_ROOT)) {
    log_err("failed loading shaders for debug_scene");
    return -1;
  }
  if (!debug_scene.make_buffers()) {
    log_err("failed creating buffers debug_scene");
    return -1;
  }
  auto [vertices, indices] = generate_bug_scene();
  debug_scene.set_data(vertices, indices);

  //
  // Lands
  //
  DebugCtx lands_dctx;
  lands::Lands lands;
  // todo: refactor to call it polyline_aa.
  roads_shader_aa::RoadsShaderAAUnit lands_aa;
  if (!lands_aa.load_shaders(SHADERS_ROOT)) {
    log_err("failed loading shaders for lands_aa");
    return -1;
  }
  if (!lands_aa.make_buffers()) {
    log_err("failed creating buffers lands_aa");
    return -1;
  }
  if (!std::getenv("DATA_ROOT")) {
    throw std::runtime_error("No DATA_ROOT specified");
  }
  const auto data_root = std::string(std::getenv("DATA_ROOT"));
  auto maybe_lands = generate_lands_quads(data_root, lands_dctx);
  if (maybe_lands) {
    auto [lands_points, lands_indices, aa_vertices, aa_indices] = *maybe_lands;
    if (!lands.load_shaders(SHADERS_ROOT)) {
      log_err("failed loading lands shaders");
      return -1;
    }
    if (!lands.make_buffers()) {
      log_err("failed making lands buffers");
      return -1;
    }
    log_debug("lands points: {}", lands_points.size());
    log_debug("lands indices: {}", lands_indices.size());
    log_debug("lands aa points: {}", aa_vertices.size());
    log_debug("lands aa indidices: {}", aa_indices.size());
    lands.set_data(lands_points, lands_indices);
    lands_aa.set_data(aa_vertices, aa_indices);
  } else {
    log_warn("Lands will not be displayed");
  }
  log_debug("There are {} debug lines for LANDS", lands_dctx.lines.size());
  // Debug context lines.
  vector<tuple<v2, v2, Color>> lines_v2;
  for (auto [a, b, c] : lands_dctx.lines) {
    lines_v2.push_back(tuple{v2{a}, v2{b}, c});
  }

  LinesUnit road_dbg_lines{(unsigned)lines_v2.size()};
  if (!road_dbg_lines.load_shaders(SHADERS_ROOT)) {
    std::cerr << "failed initializing renderer for road_dbg_lines\n";
    glfwTerminate();
    return -1;
  }
  road_dbg_lines.assign_lines(lines_v2);

  LinesUnit world_frame_lines(4);
  if (!world_frame_lines.load_shaders(SHADERS_ROOT)) {
    log_err("failed loading lines unit shaders for world frame");
    return -1;
  }
  vector<LinesUnit::line_type> world_lines;
  world_lines.emplace_back(gg::v2(0, 0), gg::v2(gg::U32_MAX, 0), colors::red);
  world_lines.emplace_back(gg::v2(gg::U32_MAX, 0),
                           gg::v2(gg::U32_MAX, gg::U32_MAX), colors::green);
  world_lines.emplace_back(gg::v2(gg::U32_MAX, gg::U32_MAX),
                           gg::v2(0, gg::U32_MAX), colors::blue);
  world_lines.emplace_back(gg::v2(0, gg::U32_MAX), gg::v2(0, 0),
                           colors::magenta);
  world_frame_lines.assign_lines(world_lines);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  bool show_debug_lines = true;
  bool show_world_bb = true;
  bool show_lands = true;
  bool show_lands_aa = true;
  bool camera_demo = false;
  bool show_debug_scene = true;
  bool show_roads = false;
  static float clear_color[4] = {1.0, 1.0, 1.0, 1.0};

  animations::AnimationsEngine animations_engine;

  enum class Scene { roads, world_lands };

  Scene scene_selected = Scene::world_lands;

  while (!glfwWindowShouldClose(window)) {

    if (camera_demo) {
      cam.zoom = cam.zoom * pow(2, std::cos(glfwGetTime()) / 2.0 * 0.01);
    }

    glfwPollEvents();
    glClearColor(clear_color[0], clear_color[1], clear_color[2],
                 clear_color[3]);
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

    triangle.render_frame(cam);
    cam_control_vis.render(cam, cam_control);

    if (show_debug_lines) {
      road_dbg_lines.render_frame(cam);
    }

    if (show_roads) {
      roads.render_frame(cam);
    }

    if (show_lands) {
      lands.render_frame(cam);
    }

    if (show_lands_aa) {
      lands_aa.render_frame(cam);
    }

    if (show_world_bb) {
      world_frame_lines.render_frame(cam);
    }

    if (show_debug_scene) {
      debug_scene.render_frame(cam);
    }

    if (g_show_crosshair) {
      crosshair.render_frame(cam);
    }

    ImGui::ListBoxHeader("Scenes", 3);
    if (ImGui::Selectable("Random Roads", scene_selected == Scene::roads)) {
      scene_selected = Scene::roads;
      log_debug("Camera goes to random roads scene...");
      show_lands = false;
      show_lands_aa = false;
      show_debug_scene = false;
      show_world_bb = false;
      show_debug_lines = false;
      show_roads = true;
      animations_engine.animate(
          &cam.focus_pos, glm::vec2(2421879040, 2732077056), 500ms, [&]() {
            animations_engine.animate(&cam.zoom, 0.000185, 500ms, []() {
              log_debug("Camera goes to random roads scene... DONE");
            });
          });
    }
    if (ImGui::Selectable("World Lands",
                          scene_selected == Scene::world_lands)) {
      scene_selected = Scene::world_lands;
      log_debug("Camera goes to World Lands scene...");
      show_roads = false;
      show_debug_scene = false;
      show_world_bb = false;
      show_lands = true;
      show_lands_aa = true;
      animations_engine.animate(
          &cam.focus_pos, glm::vec2(gg::U32_MAX / 2, gg::U32_MAX / 2), 500ms,
          [&]() {
            animations_engine.animate(
                &cam.zoom, 2.554975674209204e-07, 500ms,
                []() { log_debug("Camera goes to World Lands scene...DONE"); });
          });
    }
    ImGui::ListBoxFooter();

    ImGui::ColorEdit4("Clear color", clear_color);
    ImGui::Checkbox("Show debug lines", &show_debug_lines);
    ImGui::Checkbox("Show world BB", &show_world_bb);
    if (ImGui::Checkbox("Show Lands", &show_lands))
      ;
    ImGui::Checkbox("Show Lands AA", &show_lands_aa);
    ImGui::Checkbox("Camera Demo", &camera_demo);
    ImGui::Checkbox("Show Debug Scene", &show_debug_scene);
    ImGui::Checkbox("Show Roads", &show_roads);

    animations_engine.process_frame();

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
