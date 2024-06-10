#ifndef GLTF_IMPORTER_H_
#define GLTF_IMPORTER_H_
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <Mesh.h>

#define BUFFERSIZE 300000

/*Buffers*/
/*//TODO cambiar esto por vectores?
typedef struct{
    vec4 rotation;
    vec3 translation;
}MeshPosDataBuffer;

typedef struct {
    unsigned int numOfVertices;
    Vertex vertexData[BUFFERSIZE];
}vertexBuffer;

typedef struct {//debería tener distintos buffers para todos los tipos de indices
    unsigned int numOfIndices;
    unsigned int indices[BUFFERSIZE];
}indicesBuffer;
*/
//TODO implementar algún modo de crear la mesh con los buffers y mandar los datos al vertexBuffer

/*Model*///TODO debo cambiar esto por una clase
/*
struct Model;

typedef struct Model{
    Mesh* Meshes;//* porque tiene que guardar de verdad la info
    GLuint lastIndexAvailable;
    void (*append)(struct Model*,Mesh);//(Model *self, Mesh meshToAppend);
} Model;
void _ModelAppend(Model *self, Mesh theMesh);
void modelInit(Model *model);

void importModel(Model *model, GLuint VAO, VBO* vbo, char *modelPath, GLushort sizeofModelPath);
*/
/*desired process*/
/*

//in main.c
model model1;
initializeModel(&model1);
importModel(&model1,filePath, VAOs, VBOs);

||OR||

importModel(filePath, VAOs, VBOs); //this is only possible of the mesh DOESNT get garbage collected once the importModel functionreturns


//in gltf_importer
gets the filePath, VAOs and VBOs,
creates python process and gives it the path
python returns how many meshes to expect
c (and python) makes a loop for each mesh
    ask for numElements AND vertexData  (together as vertexBuffer)
    ask for numElements AND indices     (together as indicesBuffer)
    ask for rotationQuat and translationVec (together as meshData) (i could add to this the texture and VBO/VAO data)
    see wich VAO and VBO this mesh should use (could deduce from some data in the gltf file and the program needed)
    make a mesh and initialize it with vertexBuffer, indicesBuffer, meshData and the VAO and VBO this mesh uses
    -(i could rewrite the function in opengl_utils so that it only asks for theMesh, vertexBuffer, indicesBuffer, meshData, VAO and VBO)
    - also, mesh should store the amount of indices it has instead of getting them from sizeof(order)/sizeof(GLuint)
    vertex and indices buffers are now useless, i can destroy them 
    figure out a way to return|keep the mesh to avoid it dissapearing

python ends
c returns model

//back in main.c
frees model at the end
*/
#endif