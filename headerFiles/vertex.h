#ifndef VERTEX_H
#define VERTEX_H

#include <stdio.h>
typedef float vec3[3];
typedef float vec2[2];

typedef struct{
    vec3 pos;
    vec3 color;
    vec2 imgPos;
    vec3 normal;
}Vertex;
#endif