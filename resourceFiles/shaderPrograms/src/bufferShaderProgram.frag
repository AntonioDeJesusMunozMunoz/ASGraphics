#version 450


layout(binding = 1) uniform sampler2D albedoMaps[];//turns out no need for arbitrary number here

layout(location = 0)in vec3 color;
layout(location = 1)in vec2 imgPos;
layout(location = 2)in vec3 normal;
layout(location = 3) in mat4 modelMatrix;

layout(location = 0) out vec4 outFragColor;

layout(push_constant) uniform pushConstant{//no se pueden usar structs en uniones en glsl
	int albedoIndex;
	int matrixIndex;
}pc;

void main(){
    //outFragColor = vec4(color,1.0);
    //outFragColor = vec4(imgPos,0.0,1.0);
    //vec4 test = texture(testSampler, imgPos);

    //get albedoColor from albedoMap
    vec4 albedoColor;
    switch (pc.albedoIndex){//para no hacer esto es posible que tengas que o activar una feature en vulkan (aunq seguiria dando el error en el compilador de glsl?) o hacer testSampler un array2d de imagenes en ves de como esta ahorita
        case 0:
            albedoColor = texture(albedoMaps[0], imgPos) * vec4(color,1.0);
            break;
        case 1:
            albedoColor = texture(albedoMaps[1], imgPos) * vec4(color,1.0);
            break;
    }

    //get lightPos
    vec3 lightPos = vec3(0.0,0.0,1.0);

    //get light intensity
    float lightIntensity = clamp(dot(lightPos, vec3(modelMatrix * vec4(normal,1.0))),0, 1);

    //get ambient light
    vec3 ambientLight = vec3(0.2f,0.2f,0.2f);

    //calculate final color
    outFragColor = albedoColor * lightIntensity + vec4(ambientLight,1.0);
}