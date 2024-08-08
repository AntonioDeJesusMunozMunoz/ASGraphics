#version 450

layout(binding = 0) uniform matrixTransformations{//notese que no tiene in
    mat4 model;
    mat4 view;
    mat4 proj;
} squareMatrixTransformations;

layout(binding = 2) uniform meshMatricesBlock{//no me dejo hacerlo simplemente un array de mat4
    mat4 matrices[50];
}meshMatrices;//try using this without declaring an instance

layout(push_constant) uniform pushConstant{//no se pueden usar structs en uniones en glsl
	int albedoIndex;
	int matrixIndex;
}pc;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aImgPos;
layout(location = 3) in vec3 aNormal;

layout(location = 0) out vec3 color;
layout(location = 1) out vec2 imgPos;
layout(location = 2) out vec3 normal;
layout(location = 3) out mat4 modelMatrix;

void main(){//TODO creo que voy a pedir una model matrix por mesh ya multiplicada por la model matrix del modelo
    gl_Position = squareMatrixTransformations.proj * squareMatrixTransformations.view * squareMatrixTransformations.model * meshMatrices.matrices[pc.matrixIndex] * vec4(aPos,1.0);
    color = aColor;
    imgPos = aImgPos;
    normal = aNormal;
    modelMatrix = squareMatrixTransformations.model * meshMatrices.matrices[pc.matrixIndex];//TODO
}

/*
vec2 testTrianglePos[3] = vec2[](
	vec2(0.0,-0.3),
	vec2(0.5,0.5),
	vec2(-0.5,0.5)
);

vec3 testColors[3] = vec3[](
	vec3(0.5,1.0,0.3),
	vec3(0.8,0.7,0.3),
	vec3(0.6,0.0,0.7)
);

vec3 cpuTestTrianglePosValues[3] = vec3[](
    vec3(-0.3,-0.3,0.0),
	vec3(0.4,0.4,0.0),
	vec3(-0.5,0.3,0.0)
);

vec3 cpuTestTriangleColorValues[3] = vec3[](
    vec3(0.5, 1.0, 1.0),
	vec3(0.5, 0.0, 1.0),
	vec3(0.5, 7.0, 0.0)
);

mat4 testViewMatrix = mat4(
     1.0,  0.0, -0.0, 0.0,
    -0.0,  1.0, -0.0, 0.0,
     0.0,  0.0,  1.0, 0.0,
    -0.0, -0.0, -1.0, 1.0
);

mat4 testEmptyMatrix = mat4(
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0
);
mat4 testProjMatrix = mat4(
       1.0,       0.0,       0.0,       0.0,
       0.0,       -1.0,      0.0,       0.0,
       0.0,       0.0,       -1.020202,      -1.0,
       0.0,       0.0,       -0.202020,      0.0
);
void main(){
    gl_Position = vec4(aPos,1.0);
    color = aColor;
    bool matricesAreEqual = true;
    for(int i = 0; i < 4; i++){
		for(int j = 0; j < 4; j++){
			if(0 != ubo.model[i][j]){//if(abs(testEmptyMatrix[i][j] - ubo.view[i][j]) < 0.0001){
                matricesAreEqual = false;
            }
		}
	}
    if (!matricesAreEqual)
    {
        gl_Position = ubo.proj * ubo.view * ubo.model * vec4(aPos,1.0);//ubo.proj * ubo.view * ubo.model 
        color = testColors[1];
    }
    else
    {
        
        gl_Position = vec4(aPos,1.0);
        color = aColor;
        //gl_Position = vec4(testTrianglePos[gl_VertexIndex],0,1.0);
        //color = testColors[gl_VertexIndex];
        
    }
}*/