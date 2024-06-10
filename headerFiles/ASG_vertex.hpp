#ifndef VERTEX_H_
#define VERTEX_H_

#include <stdio.h>
typedef float vec3[3];
typedef float vec2[2];

typedef struct{
    vec3 pos;
    vec3 color;//seems to be too much if im not using really deep hdr
    vec2 imgPos;
    vec3 normal;
}Vertex;
#endif