#version 440

layout(location=1) uniform float time;
layout(location=2) uniform mat4 proj;
layout(location=3) uniform mat4 rot;
layout(location=4) uniform mat4 view;

layout(location=0) in vec4 pos;

layout(location=0) out vec4 opos;

void main() {
    opos = rot * pos;
    gl_Position = proj * view * vec4(opos.xyz / (1 - opos.w), 1.0);
}
