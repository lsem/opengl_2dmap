#include <glm/glm.hpp>

#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <limits>

#include "gg.h"
#include "glfw_helpers.h"

using namespace std;

bool g_wireframe_mode = false;
bool g_prev_wireframe_mode = false;
bool g_show_crosshair = false;

unsigned camera_x, camera_y;
double camera_scale, camera_rotation;

using p32 = gg::p32;
using v2 = gg::v2;

std::ostream& operator<<(std::ostream& os, const glm::vec3& v) {
  os << glm::to_string(v);
  return os;
}
std::ostream& operator<<(std::ostream& os, const glm::vec4& v) {
  os << glm::to_string(v);
  return os;
}

std::ostream& operator<<(std::ostream& os, const glm::vec2& v) {
  os << glm::to_string(v);
  return os;
}
std::ostream& operator<<(std::ostream& os, const glm::mat4& m) {
  os << glm::to_string(m);
  return os;
}

// // this method return geometry for triangles or triangle strips but not just
// points.
// // our shader is going to draw strips.
// std::vector<p32> generate_road_renderer(const RoadModel& road) {
//   // what does it mean to render a road?
//   // it means to produce geometry compatible with some particular shader
//   program.
//   // this geometry will be taken by another program, that prepares data for
//   shader
//   // (transfers data to GPU buffers). So overall, we want to build all roads
//   // at once.
//   // todo: open question remains: should we draw projected things or not?
// }

v2 from_glmvec2(glm::vec2 v) { return v2{v.x, v.y}; }
p32 from_v2(v2 v) {
  return p32{static_cast<uint32_t>(v.x), static_cast<uint32_t>(v.y)};
}

struct DbgContext;

class RoadRenderer {
 public:
  // loads, compiles and links shaders.
  bool prepare_shders();

  void render() {
    // use_shader();
    // bind VBA
    // draw triangles.
  }

  void add_road_geometry() {}

  static std::vector<p32> generate_geometry(vector<p32> polyline,
                                            DbgContext& ctx);
};

struct Color {
  Color(float r, float g, float b) : r(r), g(g), b(b) {}
  float r, g, b;
};

namespace colors {
static Color black{0.0f, 0.0f, 0.0f};
static Color green{0.0f, 1.0f, 0.0f};
static Color red{1.0f, 0.0f, 0.0f};
static Color blue{0.0f, 0.0f, 1.0f};
static Color grey{0.7f, 0.7f, 0.7f};
static Color magenta{1.0f, 0.0f, 1.0f};
}  // namespace colors

struct Lines {
  explicit Lines(unsigned count) {
    this->geometry.resize(count * 4);
    this->colors.resize(count * 6);
  }

  using line_type = tuple<v2, v2, Color>;

  void assign_lines(vector<line_type> lines) {
    assert(this->geometry.size() == lines.size() * 4);
    assert(this->colors.size() == lines.size() * 6);
    this->geometry.clear();
    this->colors.clear();
    for (auto [a, b, color] : lines) {
      this->geometry.push_back(a.x);
      this->geometry.push_back(a.y);
      this->geometry.push_back(b.x);
      this->geometry.push_back(b.y);
      // line has two vertex colored one way, no interpolation needed
      this->colors.push_back(color.r);
      this->colors.push_back(color.g);
      this->colors.push_back(color.b);
      this->colors.push_back(color.r);
      this->colors.push_back(color.g);
      this->colors.push_back(color.b);
    }
  }

  vector<float> geometry;
  vector<float> colors;
  int shader_program = -1;
  unsigned vao = -1, vbo = -1, colors_vbo = -1;

  void create_buffers() {
    // todo: what if I can add more data by putthing new data into separate
    // vertex object and somehow record this info in VAO?
    assert(vao == -1);
    glGenVertexArrays(1, &this->vao);
    glGenBuffers(1, &this->vbo);
    glGenBuffers(1, &this->colors_vbo);
    glBindVertexArray(this->vao);

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, geometry.size() * sizeof(geometry[0]), NULL,
                 GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2,
                          (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind

    // color
    glBindBuffer(GL_ARRAY_BUFFER, this->colors_vbo);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(colors[0]), NULL,
                 GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3,
                          (void*)0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind

    glBindVertexArray(0);  // unbind
  }

  // Supposed to be uploaded on every frame.
  void reupload_geometry() {
    if (vao == -1) {
      create_buffers();
    }
    assert(vao != -1);

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, geometry.size() * sizeof(geometry[0]),
                    &this->geometry[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind

    glBindBuffer(GL_ARRAY_BUFFER, this->colors_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(colors[0]),
                    &this->colors[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind
  }

  // todo: pass projection matrix as param.
  void render(glm::mat4 projm) {
    reupload_geometry();
    glUseProgram(this->shader_program);
    glUniformMatrix4fv(glGetUniformLocation(this->shader_program, "proj"), 1,
                       GL_FALSE, glm::value_ptr(projm));
    glBindVertexArray(this->vao);
    glDrawArrays(GL_LINES, 0, geometry.size() / 2);
  }

  const char* vertex_shader() const {
    return R"shader(
      #version 330 core
      layout (location = 0) in vec2 aPos;
      layout (location = 1) in vec3 aColor;

      out vec3 myFragColor;

      uniform mat4 proj;

      void main()
      {
        //gl_Position = proj * vec4(aPos.x + sin(aPos.y), aPos.y + sin(aPos.x), 0.0, 1.0);
        gl_Position = proj * vec4(aPos.x, aPos.y, 0.0, 1.0);
        //gl_Position = vec4(gl_Position.x + sin(gl_Position.y), gl_Position.y + sin(gl_Position.x), 0.0, 1.0);
        myFragColor = aColor;
      }
    )shader";
  }

  const char* fragment_shader() const {
    return R"shader(
      #version 330 core
      out vec4 FragColor;

      in vec3 myFragColor;

      void main()
      {
          FragColor = vec4(myFragColor, 1.0f);
      }
    )shader";
  }

  // loads and compiles shaders.
  bool load_shaders() {
    const char* v_shader_src = vertex_shader();
    const char* f_shader_src = fragment_shader();

    auto v_shader_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v_shader_id, 1, &v_shader_src, NULL);
    glCompileShader(v_shader_id);
    int result;
    char err_msg[512];
    glGetShaderiv(v_shader_id, GL_COMPILE_STATUS, &result);
    if (!result) {
      glGetShaderInfoLog(v_shader_id, 512, NULL, err_msg);
      std::cerr << "ERROR: Line: vertex shader compilation failed:\n"
                << err_msg << std::endl;
      return false;
    }
    unsigned f_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f_shader_id, 1, &f_shader_src, NULL);
    glCompileShader(f_shader_id);
    glGetShaderiv(f_shader_id, GL_COMPILE_STATUS, &result);
    if (!result) {
      glGetShaderInfoLog(f_shader_id, 512, NULL, err_msg);
      std::cerr << "ERROR: Line: fragment shader compilation failed:\n"
                << err_msg << std::endl;
      return false;
    }
    this->shader_program = glCreateProgram();
    glAttachShader(this->shader_program, v_shader_id);
    glAttachShader(this->shader_program, f_shader_id);
    glLinkProgram(this->shader_program);
    glGetProgramiv(this->shader_program, GL_LINK_STATUS, &result);
    if (!result) {
      glGetProgramInfoLog(this->shader_program, 512, NULL, err_msg);
      std::cout << "ERROR: line shaders link failed:\n" << err_msg << std::endl;
      return false;
    }
    glDeleteShader(v_shader_id);
    glDeleteShader(f_shader_id);

    return true;
  }
};

struct DbgContext {
  std::vector<tuple<p32, p32, Color>> lines;
  // if we add key then we can have layered structure with checkboxes in imgui.
  void add_line(p32 a, p32 b, Color c) { this->lines.emplace_back(a, b, c); }
  void add_line(v2 a, v2 b, Color c) {
    this->lines.emplace_back(from_v2(a), from_v2(b), c);
  }
};

// todo: it seems like it is beneficial to have precalculated normal vectors
// at map compile stage so that we have all needed points to build road from
// triangles. it now costs three normalizations (sqrt), plus atan2.
/*static*/

// so this is going to generate vertexes particularly suited for drawing
// via the same shader. In general this are vertices for this particular shader
// program. which knows how to draw roads, use specified colors and make
// antialiasing.
// See also:
//  http://wiki.gis.com/wiki/index.php/Buffer_(GIS)
std::vector<p32> RoadRenderer::generate_geometry(vector<p32> polyline,
                                                 DbgContext& ctx) {
  // to draw by two points, we need to have some special handling
  // which I have not implemented yet.
  assert(polyline.size() > 2);

  /*
   For each vertex of polyline find normalized vector that bisects angle
   between them, this would give us volume for road. tip: this probably

    ------------o d
               / \
   p1 --------o p2\
             / \   \
   ---------/e  \   \
            \    \   \
             \    \p3 \
    */

  const double WIDTH = 20.0;

  // Process polyline by overving 3 adjacent vertices
  v2 prev_d, prev_e;
  for (size_t i = 2; i < polyline.size(); ++i) {
    auto p1 = polyline[i - 2], p2 = polyline[i - 1], p3 = polyline[i];

#ifndef NDEBUG
    ctx.add_line(p1, p2, colors::black);  // original polyline
#endif

    // perpendiculars

    // t1, t2: perpendiculars to a and b
    v2 a{p1, p2};
    v2 b{p2, p3};
    v2 t1{-a.y, a.x};
    v2 t2{-b.y, b.x};

    t1 = normalized(t1) * WIDTH;
    t2 = normalized(t2) * WIDTH;

#ifndef NDEBUG
    ctx.add_line(p1, p1 + t1, colors::grey);
    ctx.add_line(p2, p2 + t1, colors::grey);
    ctx.add_line(p2, p2 + t2, colors::grey);
    ctx.add_line(p3, p3 + t2, colors::grey);

    ctx.add_line(p1, p1 + -t1, colors::grey);
    ctx.add_line(p2, p2 + -t1, colors::grey);
    ctx.add_line(p2, p2 + -t2, colors::grey);
    ctx.add_line(p3, p3 + -t2, colors::grey);
#endif // NDEBUG

    v2 d;
    if (!gg::lines_intersection(p1 + t1, p2 + t1, p3 + t2, p2 + t2, d)) {
      // in this case we can can just skip the point,
      // this must be something wrong with compilation side of the map.
      std::cout << fmt::format(fg(fmt::color::black) | bg(fmt::color::yellow),
                               "warn: parallel lines at {},{},{}", i - 2, i - 1,
                               i)
                << "\n";
      continue;
    }

    // find intersecion on other side
    v2 e;
    bool intersect = gg::lines_intersection(p1 + -t1, p2 + -t1, p3 + -t2, p2 + -t2, e);
    assert(intersect); // we already checked this on main side.

    if (i == 2) {  // first iteration
      prev_d = p1 + t1;
      prev_e = p1 + -t1;
    }

#ifndef NDEBUG
    ctx.add_line(p2, d, colors::green);
    ctx.add_line(p2, e, colors::green);
    ctx.add_line(prev_d, d, colors::blue);
    ctx.add_line(prev_e, e, colors::blue);
#endif // NDEBUG

    prev_d = d;
    prev_e = e;
  }

  // Handle end.
  auto p1 = polyline[polyline.size() - 2];
  auto p2 = polyline[polyline.size() - 1];
  v2 a{p1, p2};
  v2 perp_a = normalized(v2{-a.y, a.x}) * WIDTH;
  v2 d = p2 + perp_a;
  v2 e = p2 + -perp_a;

#ifndef NDEBUG
  ctx.add_line(p1, p2, colors::black);  // original polyline
  ctx.add_line(prev_d, d, colors::blue);
  ctx.add_line(prev_e, e, colors::blue);
#endif // NDEBUG

  return {{}};
}

namespace cameras {

struct Cam2d {
  glm::vec2 window_size;
  glm::vec2 focus_pos;
  float zoom = 1.0;
  float rotation = 0.0;
  glm::vec2 zoom_pos;

  // todo: this is view projection matrix.
  glm::mat4 projection_maxtrix() const {
    float w = this->window_size.x, h = this->window_size.y;
    auto projection_m = glm::ortho(0.0f, w, h, 0.0f);

    glm::mat4 view_m{1.0f};
    view_m =
        glm::translate(view_m, glm::vec3{w / 2.0f, h / 2.0f, 0.0f});  // 4-th
    view_m = glm::rotate(view_m, this->rotation,
                         glm::vec3{0.0f, 0.0f, -1.0f});  // 3-rd
    view_m =
        glm::scale(view_m, glm::vec3{this->zoom, this->zoom, 1.0});  // 2-nd
    view_m = glm::translate(view_m, glm::vec3{-focus_pos, 0.0f});    // 1-st
    return projection_m * view_m;
  }

  glm::vec2 unproject(glm::vec2 p) const {
    float w = this->window_size.x, h = this->window_size.y;
    auto clip_p = glm::vec4{
        p[0] / w * 2.0 - 1.0,
        (p[1] / h * 2.0 - 1.0) * -1.0,  // *-1 because we have y flipped
                                        // relative to opengl clip space
        0.0, 1.0};
    return glm::inverse(projection_maxtrix()) * clip_p;
  }
};

}  // namespace cameras

void _update_fps_counter(GLFWwindow* window) {
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

void processInput(GLFWwindow* window) {
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

const char* exp_vertex_shader = R"shader(
		#version 330 core
		layout (location = 0) in vec3 aPos;
		layout (location = 1) in float col;

		out float color;

    uniform mat4 proj;

		void main()
		{
			color = col;
		    gl_Position = proj * vec4(aPos.x + sin(aPos.y), aPos.y + sin(aPos.x), aPos.z, 1.0);
		}
)shader";

const char* exp_fragment_shader = R"shader(
		#version 330 core
		in float color;
		out vec4 FragColor;

		void main()
		{
		    FragColor = vec4(0.98, 0.8, 0.7, 1.0f);
		}
	)shader";

const char* crosshair_vshader = R"shader(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
        gl_PointSize = 30.0;
    }
)shader";
const char* crosshair_fshader = R"shader(
    #version 330 core
    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(8.0, 8.0, 8.0, 1.0f);
    }
  )shader";

unsigned g_window_width, g_window_height;

// Simple camera control for desktop applications supporting zooming around
// mouse center, rotation around desktop center and simple panning.
// todo: kinematic panning.
// todo: refactor to have just methods like (on_mouse_move(), on_mouse_click(),
// on_mouse_scroll). Basically this class maps user inputs *(mouse and keyboard)
// to camera control instructions (zoom/rotation/translations) it can even
// not be aware about camera class after all.
struct CameraControl {
  bool panning = false;
  int prev_x = 0, prev_y = 0;  // use int to be able to compare with 0.
  bool rotation = false;
  double rotation_start =
      0.0;  // camera rotation at the moment when rotation started.
  glm::vec2 rot_start_point, rot_curr_point, screen_center;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  g_window_width = width;
  g_window_height = height;

  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}

// Helper to display cam control internal stuff.
struct CamControlVis {
  Lines lines{2};

  bool init_render() {
    if (!this->lines.load_shaders()) {
      std::cerr << "error: failed loading shaders for 1-st line\n";
      return false;
    }
    return true;
  }

  // todo: idea: should we take something like rendeing_ctx just to give a
  // hint that this method uses opengl rendering context?
  void render(cameras::Cam2d& cam, CameraControl& cam_control) {
    if (cam_control.rotation) {
      this->lines.assign_lines(
          {{from_glmvec2(cam.unproject(cam_control.screen_center)),
            from_glmvec2(cam.unproject(cam_control.rot_start_point)),
            colors::grey},
           {from_glmvec2(cam.unproject(cam_control.screen_center)),
            from_glmvec2(cam.unproject(cam_control.rot_curr_point)),
            colors::grey}});
      this->lines.render(cam.projection_maxtrix());
    }
  }
};

int main() {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  g_window_width = 800, g_window_height = 600;
  GLFWwindow* window = glfwCreateWindow(g_window_width, g_window_height,
                                        "Red Triangle", nullptr, nullptr);
  if (window == nullptr) {
    std::cerr << "Failed to open GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  cameras::Cam2d cam;
  cam.window_size = glm::vec2{g_window_width, g_window_height};
  CameraControl cam_control;

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
      [&cam_control, &cam](auto* wnd, double xpos, double ypos) {
        if (cam_control.panning) {
          if (cam_control.prev_x == 0.0 && cam_control.prev_y == 0.0) {
            cam_control.prev_x = xpos;
            cam_control.prev_y = ypos;
          }
          // todo: we can just save prev already unprojected.
          auto prev_u =
              cam.unproject(glm::vec2{cam_control.prev_x, cam_control.prev_y});
          auto curr_u = cam.unproject(glm::vec2{xpos, ypos});
          cam.focus_pos -= curr_u - prev_u;

          cam_control.prev_x = xpos;
          cam_control.prev_y = ypos;
        } else if (cam_control.rotation) {
          cam_control.rot_curr_point = glm::vec2{xpos, ypos};
          // angle between these start and current vectors is our rotation.
          auto A = glm::normalize(cam_control.rot_start_point -
                                  cam_control.screen_center);
          auto B = glm::normalize(cam_control.rot_curr_point -
                                  cam_control.screen_center);
          // todo: (separate commit) why I'm not using atan2?:
          // atan2(A[1], A[0]) - atan2(B[1], B[0]);
          auto angle =
              std::acos(glm::dot(glm::normalize(A), glm::normalize(B)));
          auto Vn = glm::vec3{0.0f, 0.0f, 1.0f};
          auto crossAB = glm::cross(glm::vec3{A, 0.0}, glm::vec3{B, 0.0});
          auto signV = glm::dot(Vn, crossAB);
          if (signV < 0) {
            angle *= -1;
          }
          cam.rotation = cam_control.rotation_start + angle;
        }
      });
  glfw_helpers::GLFWMouseController::set_mouse_button_callback(
      [&cam_control, &cam](GLFWwindow* wnd, int btn, int act, int mods) {
        if (act == GLFW_PRESS && btn == GLFW_MOUSE_BUTTON_LEFT) {
          if (mods == GLFW_MOD_SHIFT) {
            double cx, cy;
            glfwGetCursorPos(wnd, &cx, &cy);
            cam_control.rotation = true;
            cam_control.rotation_start = cam.rotation;
            // we want to have this vectors used for calculation rotation in
            // screen space because after changing rotation they would be
            // invalidated.
            cam_control.rot_start_point = glm::vec2{cx, cy};
            cam_control.rot_curr_point = cam_control.rot_start_point;
            cam_control.screen_center = cam.window_size / 2.0f;  // pivot point?
          } else {
            // todo: start position can be captured here
            cam_control.panning = true;
          }
        } else if (act == GLFW_RELEASE && btn == GLFW_MOUSE_BUTTON_LEFT) {
          if (cam_control.rotation) {
            cam_control.rotation = false;
            // todo: reset rest of the state just for hygyene purposes.
          } else if (cam_control.panning) {
            cam_control.panning = false;
            cam_control.prev_x = 0;
            cam_control.prev_y = 0;
          }
        }
      });
  glfw_helpers::GLFWMouseController::set_mouse_scroll_callback(
      [&cam, &cam_control](GLFWwindow* wnd, double xoffset, double yoffset) {
        // todo: limit zoom values.
        // We want to have scrolling around mouse position. In order
        // to achieve such effect we need to keep invariant that world
        // coordinates of mouse position not changed after zooming.
        double cx, cy;
        glfwGetCursorPos(wnd, &cx, &cy);
        auto mouse_pos_world = cam.unproject(glm::vec2{cx, cy});
        cam.zoom_pos = mouse_pos_world;
        cam.zoom = cam.zoom + yoffset * 0.1;
        auto mouse_pos_after_zoom_world = cam.unproject(glm::vec2{cx, cy});
        auto diff = mouse_pos_after_zoom_world - mouse_pos_world;
        cam.focus_pos -= diff;
        cam.zoom_pos = glm::vec2{cx, cy};

#ifndef NDEBUG
        auto control_diff = cam.unproject(glm::vec2{cx, cy}) - mouse_pos_world;
        assert(control_diff[0] < 0.1 && control_diff[1] < 0.1);
        std::cout << "zoom-around-loc: sanity test passed\n";
#endif
      });
  // ------------------------------------------------------------------------

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glfwSwapInterval(1);
  glfwShowWindow(window);

  int success;
  char infoLog[512];

// ------------------------------------
#pragma pack(0)
  struct Vertex {  // todo: is it packed?
    Vertex(float x, float y, float z, float col) : x(x), y(y), z(z), col(col) {}
    float x, y, z, col;
  };
  Vertex vertices3[] = {{10000.0, 20000.0, 0.0f, 1.0f},
                        {10100.0, 20100.0, 0.0f, 0.5f},
                        {10000.0, 20200.0, 0.0f, 0.0f}};
  unsigned experimental_vao, experimental_vbo;
  glGenVertexArrays(1, &experimental_vao);
  glGenBuffers(1, &experimental_vbo);
  glBindVertexArray(experimental_vao);
  glBindBuffer(GL_ARRAY_BUFFER, experimental_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices3), &vertices3[0],
               GL_STATIC_DRAW);
  // data vec3
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void*)offsetof(Vertex, x));
  glEnableVertexAttribArray(0);
  // color
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void*)offsetof(Vertex, col));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind
  glBindVertexArray(0);              // unbind

  unsigned exp_v_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(exp_v_shader, 1, &exp_vertex_shader, NULL);
  glCompileShader(exp_v_shader);
  glGetShaderiv(exp_v_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(exp_v_shader, 512, NULL, infoLog);
    std::cout << "exp_v_shader COMPILATION_FAILED\n" << infoLog << std::endl;
    glfwTerminate();
    return -1;
  }
  unsigned exp_f_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(exp_f_shader, 1, &exp_fragment_shader, NULL);
  glCompileShader(exp_f_shader);
  glGetShaderiv(exp_f_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(exp_f_shader, 512, NULL, infoLog);
    std::cout << "exp_f_shader COMPILATION_FAILED\n" << infoLog << std::endl;
    glfwTerminate();
    return -1;
  }
  unsigned int shaderProgram2 = glCreateProgram();
  glAttachShader(shaderProgram2, exp_v_shader);
  glAttachShader(shaderProgram2, exp_f_shader);
  glLinkProgram(shaderProgram2);
  glGetProgramiv(shaderProgram2, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram2, 512, NULL, infoLog);
    std::cout << "ERROR:: shaderProgram2 Link failed\n" << infoLog << std::endl;
    glfwTerminate();
    return -1;
  }
  glDeleteShader(exp_v_shader);
  glDeleteShader(exp_f_shader);

  // ------------------------------------
  // CROSSHAIR
  // ------------------------------------
  float crosshair_vertices[] = {
      // first triangle
      0.5f, 0.5f, 0.0f,    // top right
      0.5f, -0.5f, 0.0f,   // bottom right
      -0.5f, 0.5f, 0.0f,   // top left
                           // second triangle
      0.5f, -0.5f, 0.0f,   // bottom right
      -0.5f, -0.5f, 0.0f,  // bottom left
      -0.5f, 0.5f, 0.0f    // top left
  };
  for (auto& x : crosshair_vertices) {
    x *= 0.01;
  }
  // Vertex crosshair_point = {0.0, 0.5, 0.0f, 1.0f};
  unsigned crosshair_vao, crosshair_vbo;
  glGenVertexArrays(1, &crosshair_vao);
  glGenBuffers(1, &crosshair_vbo);
  glBindVertexArray(crosshair_vao);
  glBindBuffer(GL_ARRAY_BUFFER, crosshair_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(crosshair_vertices),
               &crosshair_vertices[0], GL_STATIC_DRAW);
  // data vec3
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind
  glBindVertexArray(0);              // unbind

  unsigned crosshair_shader_id = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(crosshair_shader_id, 1, &crosshair_vshader, NULL);
  glCompileShader(crosshair_shader_id);
  glGetShaderiv(crosshair_shader_id, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(crosshair_shader_id, 512, NULL, infoLog);
    std::cout << "crosshair_shader_id COMPILATION_FAILED\n"
              << infoLog << std::endl;
    glfwTerminate();
    return -1;
  }
  unsigned crosshair_fshader_id = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(crosshair_fshader_id, 1, &crosshair_fshader, NULL);
  glCompileShader(crosshair_fshader_id);
  glGetShaderiv(crosshair_fshader_id, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(crosshair_fshader_id, 512, NULL, infoLog);
    std::cout << "crosshair_fshader_id COMPILATION_FAILED\n"
              << infoLog << std::endl;
    glfwTerminate();
    return -1;
  }
  unsigned int crosshair_shader_prog = glCreateProgram();
  glAttachShader(crosshair_shader_prog, crosshair_shader_id);
  glAttachShader(crosshair_shader_prog, crosshair_fshader_id);
  glLinkProgram(crosshair_shader_prog);
  glGetProgramiv(crosshair_shader_prog, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(crosshair_shader_prog, 512, NULL, infoLog);
    std::cout << "ERROR:: crosshair_shader_prog Link failed\n"
              << infoLog << std::endl;
    glfwTerminate();
    return -1;
  }
  glDeleteShader(crosshair_shader_id);
  glDeleteShader(crosshair_fshader_id);

  /// END CORSS

  float cx = 0.0, cy = 0.0;
  for (auto& v : vertices3) {
    cx += v.x, cy += v.y;
  }
  cx /= 3.0, cy /= 3.0;

  cam.focus_pos = glm::vec2{cx, cy};
  cam.zoom = 1.0;

  CamControlVis cam_control_vis;
  if (!cam_control_vis.init_render()) {
    glfwTerminate();
    return -1;
  }

  DbgContext dctx;

  {
    v2 origin = v2{10000.0, 20000.0} + v2{200.0, 200.0};
    auto p1 = from_v2(origin + v2{0.0, 0.0});
    auto p2 = from_v2(origin + v2{100.0, 100.0});
    auto p3 = from_v2(origin + v2{200.0, 80.0});
    auto p4 = from_v2(origin + v2{300.0, 200.0});
    auto p5 = from_v2(p4 + v2{350.0, 100.0});
    auto p6 = from_v2(p5 + v2{400.0, 100.0});
    auto p7 = from_v2(p6 + v2{30.0, -70.0});
    auto p8 = from_v2(p7 + v2{30.0, -70.0});  // parallel to previous one.
    auto p9 =
        from_v2(p8 + v2{30.0, -70.0});  // one more parallel to previous one.
    RoadRenderer::generate_geometry({p1, p2, p3, p4, p5, p6, p7, p8, p9}, dctx);
  }
  {
    v2 origin = v2{10000.0, 20000.0} + v2{1000.0, 1000.0};
    auto p1 = from_v2(origin + v2{0.0, 0.0});
    auto p2 = from_v2(p1 + v2{100.0, 0.0});
    auto p3 = from_v2(p2 + v2{30, -100.0});

    RoadRenderer::generate_geometry({p1, p2, p3}, dctx);
  }
  Lines road_dbg_lines{(unsigned)dctx.lines.size()};
  if (!road_dbg_lines.load_shaders()) {
    std::cerr << "failed initializing renderer for road_dbg_lines\n";
    glfwTerminate();
    return -1;
  }

  vector<tuple<v2, v2, Color>> lines_v2;
  for (auto [a, b, c] : dctx.lines) {
    // a,b: p32.
    // std::cout << "a,b:" << a << "; " << b << "\n";
    lines_v2.push_back(tuple{v2{a}, v2{b}, c});
  }
  road_dbg_lines.assign_lines(lines_v2);

  while (!glfwWindowShouldClose(window)) {
    _update_fps_counter(window);
    processInput(window);

    glClearColor(.9f, .9f, .9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (g_wireframe_mode != g_prev_wireframe_mode) {
      if (g_wireframe_mode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      }
    };

    glUseProgram(shaderProgram2);
    auto proj = cam.projection_maxtrix();
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "proj"), 1,
                       GL_FALSE, glm::value_ptr(proj));

    glBindVertexArray(experimental_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    if (g_show_crosshair) {
      glUseProgram(crosshair_shader_prog);
      glBindVertexArray(crosshair_vao);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    cam_control_vis.render(cam, cam_control);

    road_dbg_lines.render(cam.projection_maxtrix());

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
