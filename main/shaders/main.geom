#version 440

#define CIRCLE_RES 64
#define PI 3.14159

layout(points) in;
layout(line_strip, max_vertices=CIRCLE_RES) out;

layout(binding=1) uniform Matrices {
    mat4 proj;
    mat4 model;
};

in float xi_[];
in float eta_[];

out vec4 pos;
out vec4 pos4;
out vec4 pos3;

void main(){
    for(int k = 0; k <= CIRCLE_RES; k++) {
        vec2 xi = vec2(xi_[0], 4 * PI * k / (CIRCLE_RES - 1));
        float eta = eta_[0];

        // todo parameterize the projected circle so there aren't jagged edges.
        // should be smooth at <=32 segments, not just 128+
        float x = cos((xi.y + xi.x) / 2) * sin(eta);
        float y = sin((xi.y + xi.x) / 2) * sin(eta);
        float z = cos((xi.y - xi.x) / 2) * cos(eta);
        float w = sin((xi.y - xi.x) / 2) * cos(eta);

        pos = vec4(x, y, z, w);
        pos4 = model * pos;
        pos3 = vec4(pos4.xyz / (1 - pos4.w), 1);
        gl_Position = proj * pos3;

        if (length(gl_Position) > 4) {
            EndPrimitive();
            continue;
        }

        EmitVertex();
    }

    EndPrimitive();
}