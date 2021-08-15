#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 vColor;

uniform mat4 proj;

out vec4 color;
 
void main() {
    gl_Position = proj * vec4(aPos.x, aPos.y, 0.0, 1.0);
    color = vColor;
}
