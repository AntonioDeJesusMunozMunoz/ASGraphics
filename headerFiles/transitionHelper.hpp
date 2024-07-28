////*stuff that transitionHelper needs to see*///////


///////*stuff that main.cpp needs to see*////////
#pragma once
//dependecies
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <STB/stb_image.h>

//builtin
#define NOMINMAX //para que windows no defina max y me joda el numeric_limits
#include <stdio.h>
#include <vector>
#include <cstring>
#include <set>
#include <limits>
#include <algorithm>
#include <Array>
#include <memory>//para unique pointer
#include <map>

//local
#include <ASG_utils.hpp>
#include <ASG_vertex.hpp>
#include <ASG_swapChain.hpp>
#include <ASG_graphicsPipeline.hpp>
#include <ASGraphics.hpp>
#include <ASG_VertexIndexBuffer.hpp>
#include <ASG_imageHandler.hpp>

#include <dependencies/binFileLoader.hpp>

/*definitions echas por mi*/
#define SCREENWIDTH 500
#define SCREENHEIGTH 500
#define MAX_FRAMES_IN_FLIGHT (uint32_t)10

//clases

/*globales*/
extern std::unique_ptr<asgSwapChain> swapChain;
extern std::vector<VkFramebuffer> frameBuffers;

extern std::unique_ptr<asgPipeline> graphicsPipeline;

extern std::map<std::string, std::unique_ptr<asgVIBuffer>> materialsBuffers;

extern std::vector<VkCommandBuffer> commandBuffers;

extern VkDeviceMemory matrixUniformMemory;
extern std::vector<void*> mappedUniformBufferMemories;
extern std::vector<VkDescriptorSet> descriptorSets;

extern bool windowResized;

extern std::vector<VkSemaphore> gotframeBufferImageSemaforos;
extern std::vector<VkSemaphore>imageWrittenSemaforos;
extern std::vector<VkFence> frameDrawnFences;
extern uint32_t currFrameDrawn;

struct MatrixTransformations {
	alignas(16) glm::mat4 model;//estamos siendo explicitos con el alineamiento que queremos para evitar errores
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

/*functions*/
void createFrameBuffers();
void destroyFrameBuffers();

void remakeSwapChain();

/////*stuff not yet implemented in ASGraphics*/////