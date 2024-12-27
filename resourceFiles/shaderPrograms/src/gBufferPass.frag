#version 450

//descriptor input
layout(binding = 1) uniform sampler2D albedoMaps[];//turns out no need for arbitrary number here

//vertex shader input
layout(location = 0)in vec3 color;
layout(location = 1)in vec2 imgPos;
layout(location = 2)in vec3 normal;
layout(location = 3) in mat4 modelMatrix;

//push constants
layout(push_constant) uniform pushConstant{//no se pueden usar structs en uniones en glsl
	layout(offset = 0)int albedoIndex;
}pc;

//output
layout(location = 0) out vec4 outAlbedo;//debe ser 0
layout(location = 1) out vec4 outNormal;//debe ser 1, si no lo son, es porque estabas debuggeando


void main(){
    //get albedoColor
    switch (pc.albedoIndex){//para no hacer esto, es posible que tengas que o activar una feature en vulkan (aunq seguiria dando el error en el compilador de glsl?) o hacer testSampler un array2d de imagenes en ves de como esta ahorita
        case 0:
            outAlbedo = texture(albedoMaps[0], imgPos) * vec4(color,1.0);
            break;
        case 1:
            outAlbedo = texture(albedoMaps[1], imgPos) * vec4(color,1.0);
            break;
        case 2:
            outAlbedo = texture(albedoMaps[2], imgPos) * vec4(color,1.0);
            break;
        case 3:
            outAlbedo = texture(albedoMaps[3], imgPos) * vec4(color,1.0);
            break;
        case 4:
            outAlbedo = texture(albedoMaps[4], imgPos) * vec4(color,1.0);
            break;
    }

    //get normal
    outNormal = vec4((vec3(modelMatrix * vec4(normal,1.0))/2.0f) + vec3(0.5,0.5,0.5),1.0);//antes lo multiplicaba porla matriz modelo despues de convertirlo a colores pero creo que eso no era correct
}