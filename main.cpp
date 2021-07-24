#include <glm/glm.hpp>
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
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

struct IDisplayManager {
  virtual ~IDisplayManager() = default;
  virtual unsigned window_size_x() = 0;
  virtual unsigned window_size_y() = 0;
};

struct DisplayManager : public IDisplayManager {
  unsigned& w_ref;
  unsigned& h_ref;
  DisplayManager(unsigned& w_ref, unsigned& h_ref)
      : w_ref{w_ref}, h_ref{h_ref} {}
  virtual unsigned window_size_x() { return this->w_ref; }
  virtual unsigned window_size_y() { return this->h_ref; }
};

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

struct Line {
  vector<float> geometry{-0.5f, -0.5f, 0.5f, 0.5f};
  // vector<float> geometry{0.5f, 0.5f, 0.0f,  0.5f, -0.5f, 0.0f, -0.5f, 0.5f,
  // 0.0f};
  glm::mat4 view_projection_matrix = glm::mat4{1.0f};
  int shader_program = -1;
  unsigned vao = -1, vbo = -1;
  glm::vec2 a, b;

  void create_buffers() {
    glGenVertexArrays(1, &this->vao);
    glGenBuffers(1, &this->vbo);
    glBindVertexArray(this->vao);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, geometry.size() * sizeof(geometry[0]), NULL,
                 GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2,
                          (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind
    glBindVertexArray(0);              // unbind
  }

  // Supposed to be uploaded on every frame.
  void reupload_geometry() {
    if (vao == -1) {
      create_buffers();
    }
    assert(vao != -1);
    // todo: can I use stack here or data should be alive?
    this->geometry[0] = this->a.x;
    this->geometry[1] = this->a.y;
    this->geometry[2] = this->b.x;
    this->geometry[3] = this->b.y;

    unsigned vbo;
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, geometry.size() * sizeof(geometry[0]),
                    &this->geometry[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind
  }

  void render() {
    reupload_geometry();
    glUseProgram(this->shader_program);
    glUniformMatrix4fv(glGetUniformLocation(this->shader_program, "proj"), 1,
                       GL_FALSE, glm::value_ptr(this->view_projection_matrix));
    glBindVertexArray(this->vao);
    glDrawArrays(GL_LINES, 0, 2);
  }

  const char* vertex_shader() const {
    return R"shader(
      #version 330 core
      layout (location = 0) in vec2 aPos;

      uniform mat4 proj;

      void main()
      {
        gl_Position = proj * vec4(aPos.x, aPos.y, 0.0, 1.0);
      }
    )shader";
  }

  const char* fragment_shader() const {
    return R"shader(
      #version 330 core
      out vec4 FragColor;

      void main()
      {
          FragColor = vec4(0.7, 0.7, 0.7, 1.0f);
      }
    )shader";
  }

  // set line coords in world space
  void set_line_coords(glm::vec2 a, glm::vec2 b) {
    this->a = a;
    this->b = b;
  }

  void set_view_projection_matrix(glm::mat4 m) {
    this->view_projection_matrix = m;
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
    auto project_m = glm::ortho(0.0f, w, h, 0.0f);

#if 1
    glm::mat4 view{1.0f};
    view = glm::translate(view, glm::vec3{w / 2.0f, h / 2.0f, 0.0f});  // 4-th
    view = glm::rotate(view, this->rotation,
                       glm::vec3{0.0f, 0.0f, -1.0f});                 // 3-rd
    view = glm::scale(view, glm::vec3{this->zoom, this->zoom, 1.0});  // 2-nd
    view = glm::translate(view, glm::vec3{-focus_pos, 0.0f});         // 1-st
#else
    auto rotation_matrix = glm::rotate(glm::mat4{1.0f}, this->rotation,
                                       glm::vec3{.0f, .0f, -1.0f});
    glm::vec4 rotated_vec = rotation_matrix * glm::vec4{.0f, 1.0f, 0.0f, 1.0f};

    auto e_focus_pos = focus_pos + glm::vec2{-300.0f, -300.f};

    glm::vec3 cam_front = glm::vec3{.0f, .0f, -1.0f};
    glm::vec3 cam_up = glm::vec3{rotated_vec.x, rotated_vec.y, rotated_vec.z};
    // glm::vec3 cam_up = glm::vec4{.0f, 1.0f, 0.0f, 1.0f};
    glm::mat4 view = glm::lookAt(
        glm::vec3{e_focus_pos.x, e_focus_pos.y, 0.0f},
        cam_front + glm::vec3{e_focus_pos.x, e_focus_pos.y, 0.0f}, cam_up);
#endif
    return project_m * view;
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
		    gl_Position = proj * vec4(aPos.x, aPos.y, aPos.z, 1.0);
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
// on_mouse_scroll).
struct CameraControl {
  bool panning = false;
  int prev_x = 0, prev_y = 0;  // use int to be able to compare with 0.
  bool rotation = false;
  double rotation_start =
      0.0;  // camera rotation at the moment when rotation started.
  glm::vec2 rot_start_point, rot_curr_point;
  glm::vec2 screen_center;

  // Facilities to visualize current state of camera control state.
  struct VisualDebug {
    Line rot_start_vector_line, rot_end_vector_line;
  } vis_dbg;

  bool show_vis_dbg = false;

  bool init_render() {
    if (!this->vis_dbg.rot_start_vector_line.load_shaders()) {
      std::cerr << "error: failed loading shaders for 1-st line\n";
      return false;
    }
    if (!this->vis_dbg.rot_end_vector_line.load_shaders()) {
      std::cerr << "error: failed loading shaders for 2-nd line\n";
      return false;
    }
    return true;
  }

  // todo: idea: should we take something like rendeing_ctx just to give a hint
  // that this method uses opengl rendering context?
  void render_vis_dbg_if_enabled(cameras::Cam2d& cam) {
    if (rotation) {
      this->vis_dbg.rot_start_vector_line.set_line_coords(
          cam.unproject(this->screen_center),
          cam.unproject(this->rot_start_point));
      this->vis_dbg.rot_start_vector_line.set_view_projection_matrix(
          cam.projection_maxtrix());
      this->vis_dbg.rot_start_vector_line.render();
      this->vis_dbg.rot_end_vector_line.set_line_coords(
          cam.unproject(this->screen_center),
          cam.unproject(this->rot_curr_point));
      this->vis_dbg.rot_end_vector_line.set_view_projection_matrix(
          cam.projection_maxtrix());
      this->vis_dbg.rot_end_vector_line.render();
    }
  }

  void set_show_vis_dbg(bool v) { this->show_vis_dbg = v; }
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  g_window_width = width;
  g_window_height = height;

  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}

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

  // DisplayManager dm{g_window_width, g_window_height};
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
          auto angle =
              std::acos(glm::dot(glm::normalize(A), glm::normalize(B)));
          // now we need to find out the sign
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

  if (!cam_control.init_render()) {
    glfwTerminate();
    return -1;
  }

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

    cam_control.render_vis_dbg_if_enabled(cam);
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
