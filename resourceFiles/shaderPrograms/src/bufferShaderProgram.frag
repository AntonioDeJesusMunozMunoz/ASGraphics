#version 450

layout(binding = 1)uniform sampler2D testSampler;

layout(location = 0)in vec3 color;
layout(location = 1)in vec2 imgPos;

layout(location = 0) out vec4 outFragColor;

void main(){
    //outFragColor = vec4(color,1.0);
    //outFragColor = vec4(imgPos,0.0,1.0);
    //vec4 test = texture(testSampler, imgPos);
    outFragColor = texture(testSampler, imgPos * 2.0) * vec4(color,1.0);
}