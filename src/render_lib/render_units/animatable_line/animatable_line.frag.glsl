#version 330 core

out vec4 FragColor;

in float outT;
in float noise;

void main() {
    // float t = outT;
    float t = noise;
    t = noise;
    FragColor = vec4(t, t, t, 1);
}
