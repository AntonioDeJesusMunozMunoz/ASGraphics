//aqu� van los include que voy a necesitar para usar la libreria
//dependencies
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

#ifdef ASG_GLFW_INTEGRATION

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>//como usa el dll de glfw puedo hacerla completamente self contained

GLFWwindow* asgGetGlfwWindowHandle();
#endif

//builtins
#include <vector>
#include <string>

//local
#include <ASG_vertex.hpp>

//funciones
void asgInit();
bool asgWindowShouldClose();//literaly just glfwWindowShouldClose
void asgPollGLFWEvents();//literally just glfwPollEvents, polls the events so the OS doesnt think the aplication crashed

void asgLoadModel(std::string pathToModel);
void asgDrawFrame();
void asgTerminate();

/*tests*/
void asgLoadData(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::string materialName);
void asgLoadDataWithAlbedo(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::string albedoPath, std::string materialName);
