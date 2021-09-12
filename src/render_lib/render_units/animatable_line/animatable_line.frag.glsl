#version 330 core

out vec4 FragColor;

in float outT;
uniform float gamma;

void main() {

    float alpha = pow(outT, 1.0/gamma);
    FragColor = vec4(0.22, 0.55, 0.55, alpha);
}
