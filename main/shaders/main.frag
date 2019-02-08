#version 440

#define PI 3.14159

uniform float bright;

in vec4 pos;
in vec4 pos4;
in vec4 pos3;

out vec4 color;

void main() {
    float light = -pos3.y / 10 + .5;

    vec3 col = vec3(bright);

    //color = vec4(col * light, 1);
    color = vec4(col, 1);
}