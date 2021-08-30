#version 330 core

out vec4 FragColor;

in float t;

void main() {

    FragColor = vec4(t, t, t, 1);
}
