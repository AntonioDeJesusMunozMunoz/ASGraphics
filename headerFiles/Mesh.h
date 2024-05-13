#ifndef MESH_H
#define MESH_H

#include <stdio.h>
#include <vertex.h>
typedef float vec4[4];

typedef struct{//missing VBO byte offset?
    int VBOVertexOffset;
    unsigned int EBO;//TODO reemplazar con handle a indices buffer
    unsigned int texture;
    unsigned int textureSpec;
    unsigned int numOfIndices;
    vec4 rotation;
    vec3 translation; 
}Mesh;

#endif