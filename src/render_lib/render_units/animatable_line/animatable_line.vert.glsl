#version 330 core

layout(location = 0) in vec2 coords;
layout(location = 1) in float t;

uniform mat4 proj;
out float outT;

void main() {
    outT = t;
    gl_Position = proj * vec4(coords.x, coords.y, 0.0, 1.0);
}
