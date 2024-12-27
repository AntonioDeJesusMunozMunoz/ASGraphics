#version 450

//extensions

//attachments
layout(input_attachment_index = 0, binding = 0) uniform subpassInput albedoMap;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput normalMap;

//output
layout(location = 0) out vec4 fragColor;

void main(){
    //get actual normal
    vec3 normal = vec3((vec3(subpassLoad(normalMap).rgb) - vec3(0.5f,0.5f,0.5f)) * 2.0f);
    //calculate light intensity
    //get lightPos
    vec3 lightPos = normalize(vec3(1.0,0.0,0.0));

    //get light intensity
    float lightIntensity = clamp(dot(lightPos, normal),0, 1) + 0.15;//0.3 is ambient

    //calculate final color
    fragColor = subpassLoad(albedoMap) * lightIntensity;
}

