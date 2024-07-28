#include <ASGraphics.hpp>

//dummy stuff para meterlas a la memoria
Vertex testTriangle[] = {//pos, color
	{{0.3f,0.3f,1.0f},{0.5f, 1.0f, 1.0f}},
	{{0.4f,-0.9f,0.0f},{0.5f, 0.0f, 1.0f}},
	{{0.9f,0.3f,0.0f},{0.5f, 7.0f, 0.0f}}
};

Vertex testSquare[] = {
	{{-0.5f,-0.5f,0.0f},{0.5f, 1.0f, 1.0f},{0.0f,1.0f}},//	0----2
	{{-0.5f,0.5f,0.0f},{0.5f, 0.0f, 1.0f},{0.0f,0.0f}},//   |	 |
	{{0.5f,-0.5f,0.0f},{0.5f, 7.0f, 0.0f},{1.0f,1.0f}},//   1----3
	{{0.5f,0.5f,0.0f},{1.0f, 0.7f, 0.3f},{1.0f,0.0f}},//

	{{-0.5f,-0.7f,-0.5f},{0.5f, 1.0f, 1.0f},{0.0f,1.0f}},//		4----6
	{{-0.5f,0.3f,-0.5f},{0.5f, 0.0f, 1.0f},{0.0f,0.0f}},//   	|	 |
	{{0.5f,-0.7f,-0.5f},{0.5f, 7.0f, 0.0f},{1.0f,1.0f}},//   	5----7
	{{0.5f,0.3f,-0.5f},{1.0f, 0.7f, 0.3f},{1.0f,0.0f}}//
};

uint32_t testSquareIndices[] = {1,0,2, 1,2,3, 5,4,6, 5,6,7};

int main() {
	asgInit();
	asgLoadDataWithAlbedo(std::vector<Vertex>(std::begin(testSquare), std::end(testSquare)), std::vector<uint32_t>(std::begin(testSquareIndices), std::end(testSquareIndices)), "./resourceFiles/testImages/testImage.jpg", "test material albedo 1");
	for (int i = 0; i < 4; i++) {
		testSquare[i].pos[0] += 0.2f;
	}
	asgLoadDataWithAlbedo(std::vector<Vertex>(std::begin(testSquare), std::end(testSquare)), std::vector<uint32_t>(std::begin(testSquareIndices), std::end(testSquareIndices)), "./resourceFiles/testImages/Goty.jpg", "test material albedo 2");
	//asgLoadModel(".\\resourceFiles\\models\\bunny\\scene.gltf");

	while (!asgWindowShouldClose()) {
		asgPollGLFWEvents();
		asgDrawFrame();
	}

	asgTerminate();
	
	return 0;
}
