#version 330 core

out vec4 FragColor;


in vec4 color;

void main() {

    //FragColor = vec4(0.53, 0.54, 0.55, 1.0);
    FragColor = color;
}
