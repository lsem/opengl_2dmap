#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;

out vec3 myFragColor;

uniform mat4 proj;

void main() {
        //gl_Position = proj * vec4(aPos.x + sin(aPos.y), aPos.y + sin(aPos.x), 0.0, 1.0);
    gl_Position = proj * vec4(aPos.x, aPos.y, 0.0, 1.0);
        //gl_Position = vec4(gl_Position.x + sin(gl_Position.y), gl_Position.y + sin(gl_Position.x), 0.0, 1.0);
    myFragColor = aColor;
}
