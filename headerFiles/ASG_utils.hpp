#ifndef ASG_UTILS_H_
#define ASG_UTILS_H_

//dependencies
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

//builtin
#include <iostream>
#include <cstddef>//para std::byte
#include <vector>

/*definiciones*/
#define GRAPHICS_FAMILY_PRESENT static_cast<std::byte>((1))
#define PRESENT_FAMILY_PRESENT static_cast<std::byte>((1 << 1))
#define TRANSFER_FAMILY_PRESENT static_cast<std::byte>((1 << 2))
#define ALL_FAMILIES_PRESENT static_cast<std::byte>((GRAPHICS_FAMILY_PRESENT | PRESENT_FAMILY_PRESENT | TRANSFER_FAMILY_PRESENT))
#define MAX_FRAMES_IN_FLIGHT (uint32_t)10//aparently frames in flight = frames ready and waiting to be on screen
#define SCREENWIDTH 500
#define SCREENHEIGTH 500

/*Structs*/
struct queueFamilyIndices {
	union {
		struct {
			uint32_t graphicsFamilyIndex;
			uint32_t presentFamilyIndex;
			uint32_t transferFamilyIndex;
		};
		uint32_t allFamilyIndices[3];
	};

	std::byte familiesPresentBitMask = static_cast<std::byte>(0);

	//const functions promises to not modify the object
	bool const hasAllRequirements() {//generic check
		return (familiesPresentBitMask == ALL_FAMILIES_PRESENT);
	}
};
struct MatrixTransformations {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};
struct asgDeviceMemory {//if you end up needing more encapsulation for memory management, you should look into rewriting asg to use vulkan memory allocator, because if you dont you may be just remaking the wheel
	VkDeviceMemory handle;
	uint32_t memoryTypeIndex;
	uint32_t ocupiedBytes;
	uint32_t size;
};
struct alignas(4) pushConstants {
	union {
		struct {
			uint32_t albedoIndex;
		};
		uint32_t pbrIndices[1];
	};
	uint32_t matrixIndex;
};
union asgPbrIndices {
	struct {
		uint32_t albedoIndex;
	};
	uint32_t pbrIndices[1];
};

/////*locals*/////
extern VkPhysicalDevice physicalDevice;
extern VkDevice logicalDevice;

extern VkSurfaceKHR windowSurface;//lo usa una función de la swapchain y getSelectedQueueFamilies(osea main), ademas de momento la crea en asInit y existe la posiblidad de que la necesite yo al usarlo(o no)
extern GLFWwindow* ventana;
  
extern VkCommandPool commandPool, transferCommandPool;//las usa main y las definiciones en utils
extern VkQueue graphicsQueueHandle, presentQueueHandle, transferQueueHandle;

extern const bool validationLayersEnabled;

///////*funciones*//////
queueFamilyIndices getSelectedQueueFamilies(VkPhysicalDevice device);//lo requiero yo y la swapChain //pero se puede mejor guardar en una variable y se cre en init?
uint32_t findRigthMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryProperties);
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

VkCommandBuffer createSingleUseCmdBuffer();//podría pedir la command pool, pero de momento uso la misma siempre y no se como sincronizaria esto con endSingleUseCommandBuffer(necesita usar el mismo pool)
void endSingleUseCmdBuffer(VkCommandBuffer buffer, VkQueue queueToSubmit);//hiba a no pedir la queue pq pense q siempre era la tranfer pero no necesariamente;

//TODO para este podrías añadirle el offset que quieres que tenga como argumento y usarlo para todos los buffers o podrías usarlo solo para staging y reducir argumentos, aunque eso te forzaría a crear los uniform buffers de otra manera
void createMemoryIndependentBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags wantedMemoryProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE, uint32_t queueAmount = 1, uint32_t* queueFamilyIndices = nullptr);

void copyBuffer(VkBuffer srcBuffer, VkDeviceSize srcOffset, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size);

VkDeviceMemory allocateDeviceMemory(std::vector<VkMemoryRequirements> memoryRequirements);

VkDeviceSize calculateBuffersCombinedMemorySize(std::vector<VkMemoryRequirements> memoryRequirements);

void bindMultipleMappedBuffers(std::vector<VkMemoryRequirements> memoryRequirements, std::vector<VkBuffer> buffers, std::vector<void*> *mappedMemories, VkDeviceMemory memory);
#endif