#include <stdio.h>

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include <fileLoader.hpp>

int main() {
	printf("AAAAA");

	/*window creation*/
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);//como es vulkan tenemos que hacer algo especial para rezizable windows
	GLFWwindow* ventana = glfwCreateWindow(500,500,"uno",NULL,NULL);



	/*main loop*/

	readBinFile("./testProgram.frag.spv");
	while (!glfwWindowShouldClose(ventana)) {
		glfwPollEvents();
	}

	

	glfwDestroyWindow(ventana);
	glfwTerminate();
	return 0;
}