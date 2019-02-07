#version 440

layout(binding=1) uniform Unifs {
    mat4 uProj;
    vec4 uColor;
};

out vec4 color;

void main() {
    color = uColor;
}