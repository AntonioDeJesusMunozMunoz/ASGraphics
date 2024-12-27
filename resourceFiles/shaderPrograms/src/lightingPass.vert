#version 450

vec2 screenQuadVertices[4] = vec2[](
    vec2(-1.0f, -1.0f),
    vec2(1.0f, -1.0f),
    vec2(-1.0f, 1.0f),
    vec2(1.0f, 1.0f)
);

vec2 testScreenQuadVertices[4] = vec2[](
    vec2(-0.5f, -0.5f),
    vec2(0.5f, -0.5f),
    vec2(-0.5f, 0.5f),
    vec2(0.5f, 0.5f)
);


void main(){
    gl_Position = vec4(screenQuadVertices[gl_VertexIndex], 0.0f,1.0f);
}