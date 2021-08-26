#version 330 core

out vec4 FragColor;


in vec4 color;

void main() {

    //FragColor = vec4(0.87, 0.14, 0.38, 1);
    FragColor = color;
}
