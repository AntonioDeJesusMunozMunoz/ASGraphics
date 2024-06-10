#ifndef ASG_UTILS_H_
#define ASG_UTILS_H_

//dependencies
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//builtin
#include <iostream>
#include <cstddef>//para std::byte

/*definiciones*/
#define GRAPHICS_FAMILY_PRESENT static_cast<std::byte>((1))
#define PRESENT_FAMILY_PRESENT static_cast<std::byte>((1 << 1))
#define TRANSFER_FAMILY_PRESENT static_cast<std::byte>((1 << 2))
#define ALL_FAMILIES_PRESENT static_cast<std::byte>((GRAPHICS_FAMILY_PRESENT | PRESENT_FAMILY_PRESENT | TRANSFER_FAMILY_PRESENT))

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
/////*locals*/////
extern VkPhysicalDevice physicalDevice;
extern VkDevice logicalDevice;

extern VkSurfaceKHR windowSurface;//lo usa una función de la swapchain y getSelectedQueueFamilies(osea main), ademas de momento la crea en asInit y existe la posiblidad de que la necesite yo al usarlo(o no)
extern GLFWwindow* ventana;
  
extern VkCommandPool commandPool, transferCommandPool;//las usa main y las definiciones en utils
extern VkQueue graphicsQueueHandle, presentQueueHandle, transferQueueHandle;

///////*funciones*//////
queueFamilyIndices getSelectedQueueFamilies(VkPhysicalDevice device);//lo requiero yo y la swapChain //pero se puede mejor guardar en una variable y se cre en init?
uint32_t findRigthMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryProperties);
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

VkCommandBuffer createSingleUseCmdBuffer();//podría pedir la command pool, pero de momento uso la misma siempre y no se como sincronizaria esto con endSingleUseCommandBuffer(necesita usar el mismo pool)
void endSingleUseCmdBuffer(VkCommandBuffer buffer, VkQueue queueToSubmit);//hiba a no pedir la queue pq pense q siempre era la tranfer pero no necesariamente;
/*
void createGenericBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags wantedMemoryProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE, uint32_t queueAmount = 1, uint32_t* queueFamilyIndices = nullptr);
*/
void copyBuffer(VkBuffer srcBuffer, VkDeviceSize srcOffset, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size);
#endif