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
  }
  else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
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

  float vertices[] = {-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f};

  unsigned int VAO, VBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  //
  // Linking vertex attributes (tell OpenGL how to bind loaded vertex data to
  // vars in shader program)
  //
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

    if (g_wireframe_mode && g_wireframe_mode != g_prev_wireframe_mode) {
    	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else if (!g_wireframe_mode && g_wireframe_mode != g_prev_wireframe_mode){
    	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);  // can be not bound each time but often is like
                             // that in realistic program
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
