#version 450

//descriptor input
layout(binding = 1) uniform sampler2D albedoMaps[];//turns out no need for arbitrary number here
layout(binding = 3) uniform sampler2D lightingThresholds;//turns out no need for arbitrary number here

//vertex shader input
layout(location = 0)in vec3 color;
layout(location = 1)in vec2 imgPos;
layout(location = 2)in vec3 normal;
layout(location = 3) in mat4 modelMatrix;

//push constants
layout(push_constant) uniform pushConstant{//no se pueden usar structs en uniones en glsl
	int albedoIndex;
	int matrixIndex;
    layout(offset = 8)float randomValue;
}pc;

//output
layout(location = 0) out vec4 outFragColor;


void main(){
    //get albedoColor from albedoMap
    vec4 albedoColor;
    switch (pc.albedoIndex){//para no hacer esto es posible que tengas que o activar una feature en vulkan (aunq seguiria dando el error en el compilador de glsl?) o hacer testSampler un array2d de imagenes en ves de como esta ahorita
        case 0:
            albedoColor = texture(albedoMaps[0], imgPos) * vec4(color,1.0);
            break;
        case 1:
            albedoColor = texture(albedoMaps[1], imgPos) * vec4(color,1.0);
            break;
        case 2:
            albedoColor = texture(albedoMaps[2], imgPos) * vec4(color,1.0);
            break;
        case 3:
            albedoColor = texture(albedoMaps[3], imgPos) * vec4(color,1.0);
            break;
        case 4:
            albedoColor = texture(albedoMaps[4], imgPos) * vec4(color,1.0);
            break;
    }
    //gamma correct
    //albedoColor = vec4(pow(albedoColor.rgb, vec3(1/2.2)), albedoColor.a);//it makes it look more washed out???

    //get lightPos
    vec3 lightPos = vec3(0.0,0.0,1.0);

    //get light intensity
    float lightIntensity = clamp(dot(lightPos, vec3(modelMatrix * vec4(normal,1.0))),0, 1);

    //calculate final color
    outFragColor = albedoColor * texture(lightingThresholds, vec2(lightIntensity, pc.randomValue)).r;
}
//get ambient light//vec3 ambientLight = vec3(0.0);//vec3(0.1f,0.1f,0.1f);//it was making the colors look washed out