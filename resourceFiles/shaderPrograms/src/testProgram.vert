#version 450

//layout (location = 0) in vec3 pos;
layout(location = 0) out vec3 fragColor;

vec2 testTrianglePos[3] = vec2[](
	vec2(0.0,-0.5),
	vec2(0.5,0.5),
	vec2(-0.5,0.5)
);

vec3 testColors[3] = vec3[](
	vec3(0.5,1.0,0.3),
	vec3(0.8,0.7,0.3),
	vec3(0.6,0.0,0.7)
);

void main(){
	//gl_Position = vec4(pos,1.0);
	gl_Position = vec4(testTrianglePos[gl_VertexIndex],0.0,1.0);
	fragColor = testColors[gl_VertexIndex];
}