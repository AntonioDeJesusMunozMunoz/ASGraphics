#version 450


layout(binding = 1) uniform sampler2D testSampler[];//turns out no need for arbitrary number here

layout(location = 0)in vec3 color;
layout(location = 1)in vec2 imgPos;

layout(location = 0) out vec4 outFragColor;

layout(push_constant) uniform pushConstant{//no se pueden usar structs en uniones en glsl
	int albedoIndex;
	int matrixIndex;
}pc;

void main(){
    //outFragColor = vec4(color,1.0);
    //outFragColor = vec4(imgPos,0.0,1.0);
    //vec4 test = texture(testSampler, imgPos);
    
    switch (pc.albedoIndex){//para no hacer esto es posible que tengas que o activar una feature en vulkan (aunq seguiria dando el error en el compilador de glsl?) o hacer testSampler un array2d de imagenes en ves de como esta ahorita
        case 0:
            outFragColor = texture(testSampler[0], imgPos) * vec4(color,1.0);
            break;
        case 1:
            outFragColor = texture(testSampler[1], imgPos) * vec4(color,1.0);
            break;
    }
}