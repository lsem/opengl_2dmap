#version 330 core

out vec4 FragColor;

in float outT;

void main() {

    FragColor = vec4(0.22, 0.55, outT, 1);
}
