#ifndef ASG_SWAPCHAIN_H_
#define ASG_SWAPCHAIN_H_

//builtins
#include <vector>

//local
#include <ASG_utils.hpp>

//structs
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct asgSwapChain {
	VkSwapchainKHR handle;
	SwapChainSupportDetails supportDetails;
	VkSurfaceFormatKHR surfaceFormat;
	VkFormat depthBufferFormat;
	VkExtent2D swapExtent;
			
	std::vector<VkImage> images;//mantengo las imágenes y views separadas por que me dá más comodidad, no tienen relación 1:1 y cada image view ya me dice q image usa
	std::vector<VkImageView> views;

	//depth buffer, solo estoy creando un frame a la vez entonces 1 depth buffer me sirve
	VkImage depthBuffer;
	VkImageView depthBufferImageView;
	VkDeviceMemory depthBufferMemory;//debería juntar esto con otras

	//metodos
	asgSwapChain();
	void del();
};

//variables

//funciones
SwapChainSupportDetails getSwapChainSupportDetails(VkPhysicalDevice device);//helper de elegir physical device
void waitUntilCanRemakeSwapChain();
#endif