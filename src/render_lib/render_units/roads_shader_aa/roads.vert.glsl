#version 330 core

layout(location = 0) in vec2 coords;
layout(location = 1) in uint isOuter;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 extent_vec;

uniform mat4 proj;
uniform float scale;
uniform uint data[100];

out vec4 color;

void main() {
    //if(gl_VertexID % 2 == 1) {
    if(isOuter == 1u) { // outer
        color = vec4(inColor, 0.0);
        vec2 effective_coords = coords + extent_vec / scale;
        gl_Position = proj * vec4(effective_coords.x, effective_coords.y, 0.0, 1.0);

    } else {
        // inner
        color = vec4(inColor, 1.0);
        gl_Position = proj * vec4(coords.x, coords.y, 0.0, 1.0);
    }
}
