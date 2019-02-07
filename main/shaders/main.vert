#version 440

in vec4 iPos;

void main(){
    gl_Position = vec4(iPos.xyz, 1);
}