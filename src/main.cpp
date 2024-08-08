#define ASG_GLFW_INTEGRATION
#include <ASGraphics.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

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

struct Camera {
	glm::vec3 pos;
	glm::vec3 orientation;
	glm::vec3 up;
	float speed;
	float sensitivity;
};

const int windowWidth = 500;
const int windowHeigth = 500;

void updateCamera(GLFWwindow* ventana, Camera* cam) {

	if (glfwGetKey(ventana, GLFW_KEY_W) == GLFW_PRESS) {
		cam->pos += cam->speed * cam->orientation;
	}
	if (glfwGetKey(ventana, GLFW_KEY_S) == GLFW_PRESS) {
		cam->pos -= cam->speed * cam->orientation;
	}
	if (glfwGetKey(ventana, GLFW_KEY_A) == GLFW_PRESS) {
		cam->pos -= cam->speed * (glm::normalize(glm::cross(cam->orientation, cam->up)));
	}
	if (glfwGetKey(ventana, GLFW_KEY_D) == GLFW_PRESS) {
		cam->pos += cam->speed * (glm::normalize(glm::cross(cam->orientation, cam->up)));
	}

	double mousePos[2];
	glfwGetCursorPos(ventana, &mousePos[0], &mousePos[1]);
	if (glfwGetMouseButton(ventana, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
		glfwSetInputMode(ventana, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		cam->orientation = -glm::rotate(cam->orientation,- atanf(static_cast<float>(mousePos[0]) - windowWidth / 2.0f) * cam->sensitivity, cam->up);
		cam->orientation = -glm::rotate(cam->orientation,atanf(static_cast<float>(mousePos[1]) - windowHeigth / 2.0f) * cam->sensitivity, glm::normalize(glm::cross(cam->orientation, cam->up)));
		glfwSetCursorPos(ventana, windowWidth / 2.0f, windowHeigth / 2.0f);
	}
	else {
		glfwSetInputMode(ventana, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

int main() {
	asgInit();
	asgModelHandle bunny = asgLoadModel(".\\resourceFiles\\models\\bunny\\scene.gltf");
	asgModelHandle fish = asgLoadModel(".\\resourceFiles\\models\\BarramundiFish\\BarramundiFish.gltf");
	asgModelHandle celular = asgLoadModel(".\\resourceFiles\\models\\celular\\celular.gltf");
	
	bunny.modelMatrix = glm::translate(glm::vec3(-0.1f, 0.0f, 0.0f));
	fish.modelMatrix = glm::translate(glm::vec3(0.1f, 0.0f, 2.0f));
	celular.modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f,0.1f,0.1f));
	
	Camera primeraPersona;
	primeraPersona.pos = glm::vec3(0.0f, 0.0f, 0.0f);
	primeraPersona.orientation = glm::vec3(0.0f, 0.0f, -1.0f);
	primeraPersona.up = glm::vec3(0.0f, 1.0f, 0.0f);
	primeraPersona.speed = 0.01f;
	primeraPersona.sensitivity = 0.04f;

	while (!asgWindowShouldClose()) {
		asgPollGLFWEvents();
		updateCamera(asgGetGlfwWindowHandle(), &primeraPersona);
		glm::mat4 viewMatrix = glm::lookAt(primeraPersona.pos, primeraPersona.pos + primeraPersona.orientation, glm::vec3(0.0f, 1.0f, 0.0f));
		asgDrawFrame(viewMatrix, std::vector<asgModelHandle>({celular, bunny, fish}));
	}

	asgTerminate();
	
	return 0;
}

//for (int i = 0; i < 4; i++) {
//	testSquare[i].pos[0] += 0.2f;
//}
//asgLoadDataWithAlbedo(std::vector<Vertex>(std::begin(testSquare), std::end(testSquare)), std::vector<uint32_t>(std::begin(testSquareIndices), std::end(testSquareIndices)), "./resourceFiles/testImages/testImage.jpg", "test material albedo 1");
//asgLoadDataWithAlbedo(std::vector<Vertex>(std::begin(testSquare), std::end(testSquare)), std::vector<uint32_t>(std::begin(testSquareIndices), std::end(testSquareIndices)), "./resourceFiles/testImages/Goty.jpg", "test material albedo 2");