#include <glm/glm.hpp>
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

using namespace std;

bool g_wireframe_mode = false;
bool g_prev_wireframe_mode = false;

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
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
  else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    g_prev_wireframe_mode = g_wireframe_mode;
    g_wireframe_mode = true;
  } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
    g_prev_wireframe_mode = g_wireframe_mode;
    g_wireframe_mode = false;
  }
}

const char* vertex_shader = R"shader(
		#version 330 core
		layout (location = 0) in vec3 aPos;

		void main()
		{
		    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
		}
)shader";

const char* fragment_shader = R"shader(
		#version 330 core
		out vec4 FragColor;

		void main()
		{
		    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
		}
	)shader";

const char* exp_vertex_shader = R"shader(
		#version 330 core
		layout (location = 0) in vec3 aPos;
		layout (location = 1) in float col;

		out float color;

		void main()
		{
			color = col;
		    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
		}
)shader";
const char* exp_fragment_shader = R"shader(
		#version 330 core
		in float color;
		out vec4 FragColor;

		void main()
		{
		    FragColor = vec4(color, color, color, 1.0f);
		}
	)shader";

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
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

  // glfwDefaultWindowHints();

  GLFWwindow* window =
      glfwCreateWindow(800, 600, "Red Triangle", nullptr, nullptr);
  if (window == nullptr) {
    std::cerr << "Failed to open GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glfwSwapInterval(1);
  glfwShowWindow(window);

  // Vertex shader
  unsigned v_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(v_shader, 1, &vertex_shader, NULL);
  glCompileShader(v_shader);
  int success;
  char infoLog[512];
  glGetShaderiv(v_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(v_shader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << std::endl;
    glfwTerminate();
    return -1;
  }

  // Fragment shader
  unsigned int frag_shader;
  frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag_shader, 1, &fragment_shader, NULL);
  glCompileShader(frag_shader);

  // Link shaders
  unsigned int shaderProgram;
  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, v_shader);
  glAttachShader(shaderProgram, frag_shader);
  glLinkProgram(shaderProgram);
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cout << "ERROR:: Link failed\n" << infoLog << std::endl;
    glfwTerminate();
    return -1;
  }
  glDeleteShader(v_shader);
  glDeleteShader(frag_shader);

  float dx1 = -0.25;
  float dx2 = +0.25;
  float z = 0.0f;
  float vertices1[] = {dx1 + -0.25f, -0.25f,    z,  dx1 + 0.25f, -0.25f,
                       z,         dx1 + 0.0f, 0.25f, z};
  float vertices2[] = {dx2 + -0.25f, -0.25f,     0.0f,  dx2 + 0.25f, -0.25f,
                       0.0f,         dx2 + 0.0f, 0.25f, 0.0f};

// ------------------------------------
#pragma pack(0)
  struct Vertex {  // todo: is it packed?
    Vertex(float x, float y, float z, float col) : x(x), y(y), z(z), col(col) {}
    float x, y, z, col;
  };
  Vertex vertices3[] = {{-0.25f, -0.25f, 0.0f, 1.0f},
                        {0.25f, -0.25f, 0.0f, 0.5f},
                        {0.0f, 0.25f, 0.0f, 0.0f}};
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

  unsigned int VAO1, VBO1, VAO2, VBO2;

  // Prepare first triangle object
  glGenVertexArrays(1, &VAO1);
  glGenBuffers(1, &VBO1);
  glBindVertexArray(VAO1);
  glBindBuffer(GL_ARRAY_BUFFER, VBO1);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices1), vertices1, GL_STATIC_DRAW);
  // Linking vertex attributes (tell OpenGL how to bind loaded vertex data to
  // vars in shader program)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // unbind vao and vbo
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // Prepare second triangle object
  glGenVertexArrays(1, &VAO2);
  glGenBuffers(1, &VBO2);
  glBindVertexArray(VAO2);
  glBindBuffer(GL_ARRAY_BUFFER, VBO2);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);
  // Linking vertex attributes (tell OpenGL how to bind loaded vertex data to
  // vars in shader program)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // todo: unbind vao and vbo
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

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

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO1);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(VAO2);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glUseProgram(shaderProgram2);
    glBindVertexArray(experimental_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
