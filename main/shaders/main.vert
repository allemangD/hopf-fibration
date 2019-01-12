#version 440

layout(location=0) in float xi;
layout(location=1) in float eta;

out float xi_;
out float eta_;

void main(){
    xi_ = xi;
    eta_ = eta;
}