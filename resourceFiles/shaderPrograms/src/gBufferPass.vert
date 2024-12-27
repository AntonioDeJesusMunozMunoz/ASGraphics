#version 450
//you could embed these shaders into c++ code so that asg only need the static.lib and the header, but you are going to still develop these along side sw

//descriptor input
layout(binding = 0) uniform matrixTransformations{//notese que no tiene in
    mat4 view;
    mat4 proj;
} cameraTransforms;

layout(binding = 2) uniform meshMatricesBlock{//no me dejo hacerlo simplemente un array de mat4
    mat4 matrices[50];
}meshMatrices;//try using this without declaring an instance

//push constants
layout(push_constant) uniform pushConstant{//no se pueden usar structs en uniones en glsl
	layout(offset = 4)int matrixIndex;
}pc;

//Vertex input
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aImgPos;
layout(location = 3) in vec3 aNormal;

//output
layout(location = 0) out vec3 color;
layout(location = 1) out vec2 imgPos;
layout(location = 2) out vec3 normal;
layout(location = 3) out mat4 modelMatrix;

void main(){
    gl_Position = cameraTransforms.proj * cameraTransforms.view * meshMatrices.matrices[pc.matrixIndex] * vec4(aPos,1.0);
    color = aColor;
    imgPos = aImgPos;
    normal = aNormal;
    modelMatrix = meshMatrices.matrices[pc.matrixIndex];
}
