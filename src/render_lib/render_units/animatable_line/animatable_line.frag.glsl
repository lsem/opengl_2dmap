#version 330 core

out vec4 FragColor;

in float outT;
in float noise;

void main() {
    // float t = outT;
    float t = noise;
    t = noise;
    FragColor = vec4(0.36, 0.68, 0.66, 1);
}
