#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float col;

out float color;

uniform mat4 proj;

void main() {
    color = col;
    gl_Position = proj * vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
