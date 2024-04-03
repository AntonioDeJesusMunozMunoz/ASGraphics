#version 450

//layout (location = 0) in vec3 pos;

vec2 testTrianglePos[3] = {
	vec2(0.0,-0.5),vec2(-0.5,0.0), vec2(0.5,0.0)
};

void main(){
	//gl_Position = vec4(pos,1.0);
	gl_Position = vec4(testTrianglePos[gl_VertexIndex],0.0,1.0);
}