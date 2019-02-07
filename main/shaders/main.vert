#version 440

layout(binding=1) uniform Unifs {
    mat4 uProj;
    vec4 uColor;
};

in vec4 iPos;

void main(){
    gl_Position = uProj * vec4(iPos.xyz, 1);
}