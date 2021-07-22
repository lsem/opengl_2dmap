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

namespace cameras {

struct Cam2d {
  IDisplayManager* dm;
  glm::vec2 focus_pos;
  float zoom;
  float rotation;
  glm::vec2 zoom_pos;

  Cam2d(IDisplayManager* dm, glm::vec2 focus_pos, float zoom)
      : dm{dm}, focus_pos{focus_pos}, zoom{zoom} {}

  // todo: this is view projection matrix.
  glm::mat4 projection_maxtrix() const {
    auto project_m = glm::ortho(0.0f, (float)this->dm->window_size_x(),
                                (float)this->dm->window_size_y(), 0.0f);

    auto w = this->dm->window_size_x(), h = this->dm->window_size_y();

    // now suppose we want to have a zoom relative to point P
    glm::vec2 P{focus_pos[0] - 100, focus_pos[1] - 100};

    glm::mat4 transform{1.0f};
    transform = glm::translate(transform, glm::vec3{w / 2.0f, h / 2.0f, 0.0f});  // 4-th
    transform = glm::rotate(transform, this->rotation /* glm::radians(50.0f)*/, glm::vec3{0.0f, 0.0f, -1.0f});  // 3-rd
    transform = glm::scale(transform, glm::vec3{this->zoom, this->zoom, 1.0});  // 2-nd
    transform = glm::translate(transform, glm::vec3{-focus_pos[0], -focus_pos[1], 0.0f});  // 1-st

    return project_m * transform;
  }

  glm::vec2 unproject(glm::vec2 p) const {
    auto w = this->dm->window_size_x(), h = this->dm->window_size_y();
    auto clip_p =
        glm::vec4{p[0] / w * 2.0 - 1.0, p[1] / h * 2.0 - 1.0, 0.0, 1.0};
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
		    FragColor = vec4(1.0, 0.0, 0.0, 1.0f);
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
        FragColor = vec4(1.0, 0.0, 0.0, 1.0f);
    }
  )shader";

unsigned g_window_width, g_window_height;

struct PanningState {
  bool panning = false;
  int prev_x = 0, prev_y = 0;
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

  DisplayManager dm{g_window_width, g_window_height};
  cameras::Cam2d cam{&dm, glm::vec2{0.0, 0.0}, 1.0};
  PanningState panning_state;

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
      [&panning_state, &cam](auto* wnd, double xpos, double ypos) {
        if (panning_state.panning) {
          if (panning_state.prev_x == 0 && panning_state.prev_y == 0) {
            panning_state.prev_x = xpos;
            panning_state.prev_y = ypos;
          }
          auto dx = xpos - panning_state.prev_x;
          auto dy = ypos - panning_state.prev_y;
          panning_state.prev_x = xpos;
          panning_state.prev_y = ypos;
          // todo: fix that, rotation not working.
          cam.focus_pos += glm::vec2{-dx / cam.zoom, -dy / cam.zoom};
        }
      });
  glfw_helpers::GLFWMouseController::set_mouse_button_callback(
      [&panning_state](GLFWwindow* wnd, int btn, int act, int mods) {
        if (act == GLFW_PRESS && btn == GLFW_MOUSE_BUTTON_LEFT) {
          panning_state.panning = true;
        } else if (act == GLFW_RELEASE && btn == GLFW_MOUSE_BUTTON_LEFT) {
          panning_state.panning = false;
          panning_state.prev_x = 0;
          panning_state.prev_y = 0;
        }
      });
  glfw_helpers::GLFWMouseController::set_mouse_scroll_callback(
      [&cam, &panning_state](GLFWwindow* wnd, double xoffset, double yoffset) {
        // We want to have scrolling around mouse position. In order
        // to achieve such effect we need to keep invariant that world
        // coordinates of mouse position not changed after zooming.
        double cx, cy;
        glfwGetCursorPos(wnd, &cx, &cy);
        auto mouse_pos_world = cam.unproject(glm::vec2{cx, cy});

        std::cout << "mouse_pos_world: " << mouse_pos_world << std::endl;
        cam.zoom_pos = mouse_pos_world;

        //cam.zoom = cam.zoom + yoffset * 0.1;

        // auto mouse_pos_after_zoom_world = cam.unproject(glm::vec2{cx, cy});
        // auto diff = mouse_pos_after_zoom_world - mouse_pos_world;
        // cam.focus_pos -= diff;

        //cam.zoom_pos = glm::vec2{cx, cy};

#ifndef NDEBUG
        // auto control_diff = cam.unproject(glm::vec2{cx, cy}) - mouse_pos_world;
        // assert(control_diff[0] < 0.1 && control_diff[1] < 0.1);
        // std::cout << "zoom-around-loc: sanity test passed\n";
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
    x *= 0.02;
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

  while (!glfwWindowShouldClose(window)) {
    _update_fps_counter(window);
    processInput(window);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (g_wireframe_mode != g_prev_wireframe_mode) {
      if (g_wireframe_mode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      }
    }

    cam.rotation = std::cos(glfwGetTime()) * 2 * M_PI / 3;

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

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
