#version 440

#define CIRCLE_RES 256
#define PI 3.14159

layout(points) in;
layout(line_strip, max_vertices=CIRCLE_RES) out;

in float xi_[];
in float eta_[];

layout(binding=1) uniform Matrices {
    mat4 proj;
};

void main(){
    for(int k = 0; k <= CIRCLE_RES; k++) {
        vec2 xi = vec2(xi_[0], 4 * PI * k / CIRCLE_RES);
        float eta = eta_[0];

        float x = cos((xi.y + xi.x) / 2) * sin(eta);
        float y = sin((xi.y + xi.x) / 2) * sin(eta);
        float z = cos((xi.y - xi.x) / 2) * cos(eta);
        float w = sin((xi.y - xi.x) / 2) * cos(eta);

        vec4 pos = vec4(x / (1 - w), y / (1 - w), z / (1 - w), 1);

        gl_Position = proj * pos;
        EmitVertex();
    }
}