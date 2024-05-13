#define NOMINMAX //para que windows no defina max y me joda el numeric_limits
#include <stdio.h>
#include <iostream>
#include <cstddef>
#include <vector>
#include <cstring>
#include <set>
#include <limits>
#include <algorithm>
#include <Array>

//#define VK_USE_PLATFORM_WIN32_KHR // estos solo se usan si vas a hacer la conexion con una surface tu
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
//#define GLFW_EXPOSE_NATIVE_WIN32
//#include <glfw/glfw3native.h>

#include <binFileLoader.hpp>
#include <gltf_importer.h>
#include <STB/stb_image.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

/*definitions echas por mi*/
#define SCREENWIDTH 500
#define SCREENHEIGTH 500

#define GRAPHICS_FAMILY_PRESENT static_cast<std::byte>((1))
#define PRESENT_FAMILY_PRESENT static_cast<std::byte>((1 << 1))
#define TRANSFER_FAMILY_PRESENT static_cast<std::byte>((1 << 2))
#define ALL_FAMILIES_PRESENT static_cast<std::byte>((GRAPHICS_FAMILY_PRESENT | PRESENT_FAMILY_PRESENT | TRANSFER_FAMILY_PRESENT))

#define MAX_FRAMES_DRAWN_AT_THE_SAME_TIME (uint32_t)2

/*globales*/
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice logicalDevice{};

VkSurfaceKHR windowSurface;
VkRenderPass renderPass;
VkExtent2D chosenSwapExtent;
VkSwapchainKHR swapChain;
VkPipeline graphicsPipeline;
VkPipelineLayout pipelineLayout;
VkDescriptorSetLayout descriptorSetLayout;

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

VkCommandPool commandPool, transferCommandPool;
std::vector<VkCommandBuffer> commandBuffers(MAX_FRAMES_DRAWN_AT_THE_SAME_TIME);
VkQueue graphicsQueueHandle, presentQueueHandle, transferQueueHandle;

VkDescriptorPool descriptorPool;
std::vector<VkBuffer> uniformBuffers(MAX_FRAMES_DRAWN_AT_THE_SAME_TIME);
std::vector<VkDeviceMemory> uniformBufferMemories(MAX_FRAMES_DRAWN_AT_THE_SAME_TIME);
std::vector<void*> mappedUniformBufferMemories(MAX_FRAMES_DRAWN_AT_THE_SAME_TIME);
std::vector<VkDescriptorSet> descriptorSets(MAX_FRAMES_DRAWN_AT_THE_SAME_TIME);

GLFWwindow* ventana;
bool windowResized = false;

std::vector<VkSemaphore> gotframeBufferImageSemaforos(MAX_FRAMES_DRAWN_AT_THE_SAME_TIME);
std::vector<VkSemaphore>imageWrittenSemaforos(MAX_FRAMES_DRAWN_AT_THE_SAME_TIME);
std::vector<VkFence> frameDrawnFences(MAX_FRAMES_DRAWN_AT_THE_SAME_TIME);
uint32_t currFrameDrawn = 0;
const std::vector<const char*> usedExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};//es un typedef string
const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

std::vector<VkImage> swapChainImages;
std::vector<VkImageView> swapChainImageViews;
std::vector<VkFramebuffer> frameBuffers;

VkImage depthBuffer;
VkDeviceMemory depthBufferMemory;
VkImageView depthBufferImageView;

VkImage testImage;
VkDeviceMemory testImageMemory;

struct MatrixTransformations {
	alignas(16) glm::mat4 model;//estamos siendo explicitos con el alineamiento que queremos para evitar errores
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

//dummy shapes para meterlas a la memoria
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

#ifdef NDEBUG
	const bool validationLayersEnabled = false;
#else
	const bool validationLayersEnabled = true;
#endif

/*Structs*/
struct SwapChainSupportDetails{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};
struct queueFamilyIndices{
	union{
			struct{
				uint32_t graphicsFamilyIndex;
				uint32_t presentFamilyIndex;
				uint32_t transferFamilyIndex;
			};
		uint32_t allFamilyIndices[3];
	};

	std::byte familiesPresentBitMask = static_cast<std::byte>(0);

	bool hasAllRequirements(){//generic check
		return (familiesPresentBitMask == ALL_FAMILIES_PRESENT);
	}
};

/*funciones*/
SwapChainSupportDetails getSwapChainSupportDetails(VkPhysicalDevice device){
		/*hacemos la swap chain*/
	
	///obtenemos toda la info de la swapchain
	SwapChainSupportDetails swapChainSupportInfo{};
	
	//surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, windowSurface, &swapChainSupportInfo.capabilities);
	
	//surface formats
	uint32_t formatAmount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, windowSurface, &formatAmount, nullptr);	
	if (formatAmount != 0){
	swapChainSupportInfo.formats.resize(formatAmount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, windowSurface, &formatAmount, swapChainSupportInfo.formats.data());	
	}

	//surface present modes
	uint32_t presentModesAmount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, windowSurface, &presentModesAmount, nullptr);
	if(presentModesAmount != 0){
	swapChainSupportInfo.presentModes.resize(presentModesAmount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, windowSurface, &presentModesAmount, swapChainSupportInfo.presentModes.data());
	}

	return swapChainSupportInfo;
}

queueFamilyIndices getSelectedQueueFamilies(VkPhysicalDevice device){
	queueFamilyIndices selectedQueueFamilies{};

	//conseguimos queueFamilyProperties de la misma manera
	uint32_t queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queueFamiliesProperties.data());

	//checamos bitwise si tiene queue de gráficos
	for (int i = 0; i < queueCount; i++){
		if(queueFamiliesProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){//graphics and compute families implicitly have transfer capabilities, i wanted a challenge so i used a third diferent queue
			selectedQueueFamilies.graphicsFamilyIndex = i;
			selectedQueueFamilies.familiesPresentBitMask |= GRAPHICS_FAMILY_PRESENT;
		}else if (queueFamiliesProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT){
			selectedQueueFamilies.transferFamilyIndex = i;
			selectedQueueFamilies.familiesPresentBitMask |= TRANSFER_FAMILY_PRESENT;
			continue;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, windowSurface, &presentSupport);
		if (presentSupport){
			selectedQueueFamilies.presentFamilyIndex = i;
			selectedQueueFamilies.familiesPresentBitMask |= PRESENT_FAMILY_PRESENT;
		}

		if (selectedQueueFamilies.hasAllRequirements()){
			break;
		}
	}

	return selectedQueueFamilies;
}

VkSurfaceFormatKHR getSwapChainSurfaceFormat(SwapChainSupportDetails swapChainInfo){
	VkSurfaceFormatKHR chosenSwapSurfaceFormat = swapChainInfo.formats[0];// si no encuetro el que quiero, agarro el primero

	for (const auto& currFormat: swapChainInfo.formats){
		if(currFormat.format == VK_FORMAT_B8G8R8A8_SRGB && currFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
			chosenSwapSurfaceFormat = currFormat;
			printf("format chosen succesfully\n");
			break;
		}
	}
	return chosenSwapSurfaceFormat;
}

//Buffer functions
uint32_t findRigthMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryProperties){
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

	for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++){
		if(((1 << i) & typeFilter)  // 1 << i: el bit en la posición i le ponemos valor 1 y checamos si es el que buscamos, literal es solo bitwise curr == wanted
		&& (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties){//checo si memoryProperties y currPhysicalMemoryProperties tienen las mismas flags
			return i;
		}
	}

	throw std::runtime_error("could not find suitable memory type");
}

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){
	//alojamos comand buffer para la transferencia
	VkCommandBuffer transferCommandBuffer;
	VkCommandBufferAllocateInfo commandBufferAI{};
	commandBufferAI.commandBufferCount = 1;
	commandBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAI.commandPool = transferCommandPool;
	
	if(vkAllocateCommandBuffers(logicalDevice, &commandBufferAI, &transferCommandBuffer) != VK_SUCCESS){
		throw std::runtime_error("could not create transfer command buffer");
	}

	//lo iniciamos
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;//le decimos que solo se usará una vez

	vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

	//copiamos
	VkBufferCopy copyRegion{};
	copyRegion.dstOffset = 0;
	copyRegion.srcOffset = 0;
	copyRegion.size = size;

	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, & copyRegion);

	vkEndCommandBuffer(transferCommandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	vkQueueSubmit(transferQueueHandle, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueueHandle);//esperamos a que se termine de copiar

	//vkFreeCommandBuffers(logicalDevice, commandPool, 1, &transferCommandBuffer);//quizá no necesito liberarlo porque lo libera al destruir su pool

}

void createBuffer(VkDeviceSize size,VkBufferUsageFlags usage, VkMemoryPropertyFlags wantedMemoryProperties, VkBuffer *buffer, VkDeviceMemory *bufferMemory, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE, uint32_t queueAmount = 1, uint32_t* queueFamilyIndices = nullptr){
	//llenado
	VkBufferCreateInfo bufferCI{};
	bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

	bufferCI.queueFamilyIndexCount = queueAmount;
	bufferCI.pQueueFamilyIndices = queueFamilyIndices;//estos son para especificar que families comparten este buffer

	bufferCI.sharingMode = sharingMode;//lo usará la transfer y graphics queue, all usar diferente queue para staging buffer puede que vea beneficios en velocidad(más queues es similar a más threads, más rápido, pero solo si no hay demasiada espera para sincronizarlas), también SHARING_MODE_CONCURRENT puede ser más lento que SHARING_MODE_EXCLUSIVE, el industry standard es una queue que solo se dedica a transfers host->driver
	bufferCI.size = size;
	bufferCI.usage = usage;
	
	if(vkCreateBuffer(logicalDevice, &bufferCI, nullptr, buffer) != VK_SUCCESS){
		throw std::runtime_error("could not createBuffer");
	}	

	//alojar memoria
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findRigthMemoryType(memoryRequirements.memoryTypeBits, wantedMemoryProperties);//queremos una memoria que el host pueda ver y sea coherente con el

	if(vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, bufferMemory) != VK_SUCCESS){
		throw std::runtime_error("could not allocate vertex buffer memory");
	}

	//conectar la memoria al vertex buffer
	vkBindBufferMemory(logicalDevice, *buffer, *bufferMemory, 0);//si el offset no fuera 0, debería ser divisible entre memoryRequirements.alignment

}

void createIndexBuffer(){
	//crear staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(sizeof(testSquareIndices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stagingBuffer, &stagingBufferMemory);

	//mapear la memoria
	void* data;
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, sizeof(testSquareIndices), 0, &data);//también podría poner VK_WHOLE_SIZE para mapear toda la memoria
	memcpy(data, testSquareIndices, sizeof(testSquareIndices));//el simple echo de hacer memcpy no garantiza que el driver lo copie (ej: caching), tonce podríamos flushearlo manualmente o asegurarnos que esta memoria sea coherente con el host(osea que el mapa y la memoria en gpu siempre sean la misma)
	vkUnmapMemory(logicalDevice, stagingBufferMemory);
	//mover los datos al gpu ocurre en el fondo, vulkan solo dice que está garantizado que ya haya ocurrido para la siguiente llamada a vkQueueSubmit

	//crear index buffer
	queueFamilyIndices selectedQueueFamilies =  getSelectedQueueFamilies(physicalDevice);
	uint32_t indexBufferQueues[] = {selectedQueueFamilies.graphicsFamilyIndex, selectedQueueFamilies.transferFamilyIndex};
	createBuffer(sizeof(testSquareIndices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&indexBuffer, &indexBufferMemory, VK_SHARING_MODE_CONCURRENT, 2, indexBufferQueues);

	copyBuffer(stagingBuffer, indexBuffer, sizeof(testSquareIndices));

	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	
}

void createVertexBuffer(){
	//crear staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(sizeof(testSquare), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stagingBuffer, &stagingBufferMemory);

	//mapear la memoria
	void* data;
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, sizeof(testSquare), 0, &data);//también podría poner VK_WHOLE_SIZE para mapear toda la memoria
	memcpy(data, testSquare, sizeof(testSquare));//el simple echo de hacer memcpy no garantiza que el driver lo copie (ej: caching), tonce podríamos flushearlo manualmente o asegurarnos que esta memoria sea coherente con el host(osea que el mapa y la memoria en gpu siempre sean la misma)
	vkUnmapMemory(logicalDevice, stagingBufferMemory);
	//mover los datos al gpu ocurre en el fondo, vulkan solo dice que está garantizado que ya haya ocurrido para la siguiente llamada a vkQueueSubmit

	//crear vertex buffer
	queueFamilyIndices selectedQueueFamilies =  getSelectedQueueFamilies(physicalDevice);
	uint32_t vertexBufferQueues[] = {selectedQueueFamilies.graphicsFamilyIndex, selectedQueueFamilies.transferFamilyIndex};
	createBuffer(sizeof(testSquare), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&vertexBuffer, &vertexBufferMemory, VK_SHARING_MODE_CONCURRENT, 2, vertexBufferQueues);

	copyBuffer(stagingBuffer, vertexBuffer, sizeof(testSquare));

	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	
}

//SwapChain

VkCommandBuffer createSingleUseCmdBuffer(){//Aloja e inicia un command buffer con ONE_TIME_SUBMIT flag
	VkCommandBufferAllocateInfo cmdBufferAI{};
	cmdBufferAI.commandBufferCount = 1;
	cmdBufferAI.commandPool = commandPool;
	cmdBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

	VkCommandBuffer returnCmdBuffer;
	vkAllocateCommandBuffers(logicalDevice, &cmdBufferAI, &returnCmdBuffer);

	VkCommandBufferBeginInfo cmdBufferBI{};
	cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(returnCmdBuffer, &cmdBufferBI);
	return returnCmdBuffer;
}

void endSingleUseCmdBuffer(VkCommandBuffer buffer){
	vkEndCommandBuffer(buffer);

	VkSubmitInfo SI{};
	SI.commandBufferCount = 1;
	SI.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SI.pCommandBuffers = &buffer;
	vkQueueSubmit(graphicsQueueHandle, 1, &SI, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueueHandle);

	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &buffer);
}


void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout){
	VkImageMemoryBarrier barrier{};
	barrier.oldLayout = oldLayout;//este podría ser VK_IMAGE_LAYOUT_UNDEFINED si no te importa el contenido existente en la imagen
	barrier.newLayout = newLayout;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;//ps no estamos cambiando la ownership de la queue family
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;// solo tengo uno de estos
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	VkPipelineStageFlags srcStage, dstStage;
	if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){//estos atributos de la barrera dependen de la transición
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;//Detendremos el write
		barrier.srcAccessMask = 0;//hasta que nada (osea no lo detenemos)

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;//Entre el inicio y transfer(ponemos el inicio pq ps no esperamos nada)
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;//Detendremos el read
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;//hasta que tranfer termine de escribir
		
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;//Entre transfer y fragment
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;//detengo las operaciones con 
		barrier.srcAccessMask = 0;
		
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;//Entre inicio y los primeros tests de fragmentos
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;
	}else{
		throw std::runtime_error("called layoutTransition with a layout pair that isnt supported by the if");
	}
	//Transfer no es una stage real, es simplemente una "stage" en donde transfers ocurren

	VkCommandBuffer commandBuffer = createSingleUseCmdBuffer();

	

	vkCmdPipelineBarrier(commandBuffer,
						srcStage,//En que pipeline stage ocurren las operaciones que deben ocurrir antes de la barrera
						dstStage, //En que pipeline stage ocurren las operaciones que deben esperar a la barrera
						0,//la única otra opción es VK_DEPENDENCY_BY_REGION_BIT que permite leer de las regiones donde ya se haya escrito
						0, nullptr, 0, nullptr, 1, &barrier);

	
	endSingleUseCmdBuffer(commandBuffer);
}

//const ... &candidates, const es para comunicar que no le moveré, & hace que me den la addres de la variable, lo que lo hace tecnicamente un pointer, pero usar & en vez de * dice que no quiero moverle a la variable original, solo no quiero que copie todos los contenidos de la variable al mandarlo a la función
VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling wantedTiling, VkFormatFeatureFlags wantedFeatureFlags){
	for (VkFormat currFormat: candidates){
		//conseguimos las propiedades de los formatos que son supported por el physical device
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, currFormat, &formatProperties);
		switch (wantedTiling){
			case VK_IMAGE_TILING_LINEAR:
				if ((formatProperties.linearTilingFeatures & wantedFeatureFlags) == wantedFeatureFlags){
					return currFormat;
				}
				break;

			case VK_IMAGE_TILING_OPTIMAL:
				if ((formatProperties.optimalTilingFeatures & wantedFeatureFlags) == wantedFeatureFlags){
					return currFormat;
				}
				break;

			default:
				printf("the wanted tiling was not supported by the switch");
				break;
		}
	}
	throw std::runtime_error("Could not find a supported format");
}



void createImageViews(){
	swapChainImageViews.resize(swapChainImages.size());
	SwapChainSupportDetails swapChainInfo = getSwapChainSupportDetails(physicalDevice);
	VkSurfaceFormatKHR chosenSwapSurfaceFormat = getSwapChainSurfaceFormat(swapChainInfo);

	for (size_t i = 0; i < swapChainImages.size(); i++){
		VkImageViewCreateInfo currImageViewInfo{};
		currImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		currImageViewInfo.image = swapChainImages[i];

		currImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		currImageViewInfo.format = chosenSwapSurfaceFormat.format;
		//estos te permiten mucha customización de los color channels
		currImageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		currImageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		currImageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		currImageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		currImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;//indicamos que estas imágenes son color targets
		currImageViewInfo.subresourceRange.baseArrayLayer = 0;// no queremos mip maps ni multiple layers
		currImageViewInfo.subresourceRange.baseMipLevel = 0;
		currImageViewInfo.subresourceRange.layerCount = 1;
		currImageViewInfo.subresourceRange.levelCount = 1;
		if(vkCreateImageView(logicalDevice, &currImageViewInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS){
			throw std::runtime_error("could not create Image Views");
		}else{
			printf("Image view %u created\n", i);
		}
	}
}

void createFrameBuffers(){
	frameBuffers.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImageViews.size(); i++){//creamos framebuffer de cada color attachment  
		VkImageView currAttachments[] = {swapChainImageViews[i], depthBufferImageView};//solo es el de color pero ps luego le meteremos mas attachments //podemos usar el mismo depthBuffer porque solo una subpass ocurre simultaneamente debido a nuestros semáforos
		VkFramebufferCreateInfo currCI{};
		currCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		currCI.renderPass = renderPass; //decimos que debe ser compatible con esta render pass
		currCI.attachmentCount = 2;
		currCI.pAttachments = currAttachments;
		currCI.width = chosenSwapExtent.width;
		currCI.height = chosenSwapExtent.height;
		currCI.layers = 1; 

		if(vkCreateFramebuffer(logicalDevice, &currCI, nullptr, &(frameBuffers[i])) != VK_SUCCESS){
			throw std::runtime_error("could not create frame buffer");
		}
	}
}

void destroySwapChain(){
	for (auto currFrameBuffer : frameBuffers){//debemos destruirlo antes de la render pass e image views
		vkDestroyFramebuffer(logicalDevice, currFrameBuffer, nullptr);
	}
	for (auto imageView : swapChainImageViews){
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	}
	vkDestroyImage(logicalDevice, depthBuffer, nullptr);
	vkDestroyImageView(logicalDevice, depthBufferImageView,nullptr);
	vkFreeMemory(logicalDevice, depthBufferMemory, nullptr);

	vkDestroySwapchainKHR(logicalDevice,swapChain,nullptr);
}

void createSwapChain(){
	//elegimos surface format
	SwapChainSupportDetails swapChainInfo = getSwapChainSupportDetails(physicalDevice);
	VkSurfaceFormatKHR chosenSwapSurfaceFormat = getSwapChainSurfaceFormat(swapChainInfo);

	//elegimos presentation mode
	VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;//default

	for (const auto& currPresentMode : swapChainInfo.presentModes){
		if (currPresentMode == VK_PRESENT_MODE_MAILBOX_KHR){
			chosenPresentMode = currPresentMode;
		}
	}

	//elegimos swap extent
	chosenSwapExtent = swapChainInfo.capabilities.currentExtent;//default
	if (swapChainInfo.capabilities.currentExtent.width == (uint32_t)std::numeric_limits<uint32_t>::max()){
		//conseguimos el tamaño en píxeles de la ventana
		int pixelWidth, pixelHeigth;
		glfwGetFramebufferSize(ventana,&pixelWidth, &pixelHeigth);
		chosenSwapExtent = {static_cast<uint32_t>(pixelWidth), static_cast<uint32_t>(pixelHeigth)};

		//lo clampeamos y lo regresamos
		chosenSwapExtent.width = std::clamp(chosenSwapExtent.width, swapChainInfo.capabilities.minImageExtent.width, swapChainInfo.capabilities.maxImageExtent.width);
		chosenSwapExtent.height = std::clamp(chosenSwapExtent.height, swapChainInfo.capabilities.minImageExtent.height, swapChainInfo.capabilities.maxImageExtent.height);
	}

	//last swapChain detail: image count
	uint32_t swapChainImageCount = swapChainInfo.capabilities.minImageCount + 1;

	if (swapChainInfo.capabilities.maxImageCount != 0 && swapChainImageCount > swapChainInfo.capabilities.maxImageCount){
		swapChainImageCount = swapChainInfo.capabilities.maxImageCount; 
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = windowSurface;

	swapChainCreateInfo.imageColorSpace = chosenSwapSurfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = chosenSwapExtent;
	swapChainCreateInfo.imageFormat = chosenSwapSurfaceFormat.format;
	swapChainCreateInfo.presentMode = chosenPresentMode; 
	swapChainCreateInfo.imageArrayLayers = 1; // amount of layers an image has, always one unless developing stereoscopic 3d
	swapChainCreateInfo.minImageCount = swapChainImageCount;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;//we are renndering directly to them so they are color attachment
	//the one you would use for drawing them somewhere else and postprocessing would be VK_IMAGE_USAGE_TRANSFER_DST_BIT

	queueFamilyIndices selectedQueueFamilies = getSelectedQueueFamilies(physicalDevice);

	if (selectedQueueFamilies.graphicsFamilyIndex != selectedQueueFamilies.graphicsFamilyIndex){
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;//these 2 args specify wich queue families will share the images
		swapChainCreateInfo.pQueueFamilyIndices = selectedQueueFamilies.allFamilyIndices;
	}else{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;//optional
		swapChainCreateInfo.pQueueFamilyIndices = nullptr; //optional
	}

	swapChainCreateInfo.preTransform = swapChainInfo.capabilities.currentTransform;//we can specify a transformation to apply to all images, here we are specifying we dont want any
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;//should the alpha be used to blend with other windows in the windows system, almost always no
	swapChainCreateInfo.clipped = VK_TRUE;//clip obscured pixels, like by other windows
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if(vkCreateSwapchainKHR(logicalDevice, &swapChainCreateInfo, nullptr, &swapChain) != VK_SUCCESS){
		throw std::runtime_error("could not create swap chain");
	}else{
		printf("swap chain created\n");
	}
	
	//obtenemos las handles de las imagenes, vulkan puede crear las que quiera así que hay que pedir cuantas son
	uint32_t amountOfSwapChainImages = 0;
	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &amountOfSwapChainImages, nullptr);

	swapChainImages.resize(amountOfSwapChainImages);
	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &amountOfSwapChainImages, swapChainImages.data());

}

void remakeSwapChain(){
	int currScreenWidth = 0, currScreenHeigth = 0;
	glfwGetFramebufferSize(ventana, &currScreenWidth, &currScreenHeigth);
	printf("\ncurrScreenWidth; %i, currScreenHeigth: %i\n", currScreenWidth,currScreenHeigth);
	while(currScreenWidth == 0 || currScreenHeigth == 0){//mientras que la ventana mide 0x0 (minimizada), espero
		glfwGetFramebufferSize(ventana, &currScreenWidth, &currScreenHeigth);
		glfwWaitEvents();//espera a que ocurra algo referente a la ventana antes de checar su tamaño otra vez
	}

	vkDeviceWaitIdle(logicalDevice);//primero espero a que todas las async op terminen, tecnicamente podría recrear la swapChain dandole la anterior en oldSwapChain para que termine de ejecutar las ops de la antigua

	destroySwapChain();

	createSwapChain();
	createImageViews();
	/*depth buffer*///debe estar debajo de create swap chain y arriba de create framebuffers
	//imagen
	VkFormat supportedDepthBufferFormat = findSupportedFormat({VK_FORMAT_D32_SFLOAT_S8_UINT,VK_FORMAT_D24_UNORM_S8_UINT},VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	VkImageCreateInfo depthImageCI{};
	depthImageCI.arrayLayers = 1;
	depthImageCI.extent = {chosenSwapExtent.width, chosenSwapExtent.height, 1};
	depthImageCI.flags = 0;//Opcional par amemory eficiency
	depthImageCI.format = supportedDepthBufferFormat;
	depthImageCI.imageType = VK_IMAGE_TYPE_2D;
	depthImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthImageCI.mipLevels = 1;
	depthImageCI.pQueueFamilyIndices = nullptr;//Optional
	depthImageCI.queueFamilyIndexCount = 0;
	depthImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	depthImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	depthImageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depthImageCI.tiling = 
	VK_IMAGE_TILING_OPTIMAL;
	depthImageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	
	if( vkCreateImage(logicalDevice, &depthImageCI, nullptr, &depthBuffer) != VK_SUCCESS){
		throw std::runtime_error("could not create depth buffer image");
	}

	//memory
	VkMemoryRequirements depthBufferMemoryRequirements;
	vkGetImageMemoryRequirements(logicalDevice, depthBuffer, &depthBufferMemoryRequirements);

	VkMemoryAllocateInfo depthBufferMemoryAI{};
	depthBufferMemoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	depthBufferMemoryAI.allocationSize = depthBufferMemoryRequirements.size;
	depthBufferMemoryAI.memoryTypeIndex = findRigthMemoryType(depthBufferMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	vkAllocateMemory(logicalDevice, &depthBufferMemoryAI, nullptr, &depthBufferMemory);

	vkBindImageMemory(logicalDevice, depthBuffer, depthBufferMemory, 0);

	//image view
	//necesito checar si el formato que quiero tiene support de depth y stencil
	//VkFormat supportedDepthBufferFormat = findSupportedFormat({VK_FORMAT_D32_SFLOAT_S8_UINT,VK_FORMAT_D24_UNORM_S8_UINT},VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	VkImageViewCreateInfo depthBufferImageViewCI{};
	depthBufferImageViewCI.format = supportedDepthBufferFormat;
	depthBufferImageViewCI.image = depthBuffer;
	depthBufferImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

	depthBufferImageViewCI.subresourceRange.baseArrayLayer = 0;
	depthBufferImageViewCI.subresourceRange.baseMipLevel = 0;
	depthBufferImageViewCI.subresourceRange.layerCount = 1;
	depthBufferImageViewCI.subresourceRange.levelCount = 1;
	depthBufferImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

	depthBufferImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthBufferImageViewCI.flags = 0;
	vkCreateImageView(logicalDevice, &depthBufferImageViewCI,nullptr, &depthBufferImageView);

	//ahora que ya creé los recursos para el depth buffer debo de cambiar su layout
	transitionImageLayout(depthBuffer, supportedDepthBufferFormat,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	createFrameBuffers();
	
}


void grabarCommandBuffer(VkCommandBuffer buffer, uint32_t imageIndex){
	//begin recording
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;//opcional
	beginInfo.pInheritanceInfo = nullptr;//es para buffers secundarios

	if(vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS){//lo resetea implicitamente
		throw std::runtime_error("could not begin recording commands");
	}

	/*begin render pass*/
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = frameBuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0,0};
	renderPassInfo.renderArea.extent = chosenSwapExtent;

	//clear value
	VkClearValue clearValues[2];//EL ORDEN DE ESTOS CLEAR VALUES DEBE SER IDENTICO AL DE LOS ATTACHMENTS
	clearValues[0].color = {0.0f,0.0f,0.0f};//POSIBLE BUG SOURCE PQ NO INICALIZO LOS STRUCTS
	clearValues[1].depthStencil = {1.0f, 0};//los pongo todos en 1.0f, el valor de profundidad más lejano, el 0 es stencil

	

	renderPassInfo.clearValueCount = 2;
	renderPassInfo. pClearValues = clearValues;

	//iniciar la render pass
	vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);//3er argumento, los comandos de la render pass serán embedded en el buffer sin usar un buffer secundario
	vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkBuffer buffers[] = {vertexBuffer};
	VkDeviceSize bufferOffsets[] = {0};
	vkCmdBindVertexBuffers(buffer, 0, 1, buffers, bufferOffsets);
	vkCmdBindIndexBuffer(buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currFrameDrawn], 0, nullptr);//tengo que especificar si a la graphics o compute pipeline
	//ViewPort y Scissors, pq los pusimos como dynamic state
	VkViewport viewport{};
	viewport.height = static_cast<float>(chosenSwapExtent.height);
	viewport.width = static_cast<float>(chosenSwapExtent.width);
	viewport.maxDepth = 1.0f;
	viewport.minDepth = 0.0f;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	vkCmdSetViewport(buffer, 0, 1, &viewport);

	VkRect2D tijeras{};
	tijeras.extent = chosenSwapExtent;
	tijeras.offset = {0,0};
	vkCmdSetScissor(buffer, 0, 1, &tijeras);

	//draw and end
	//vkCmdDraw(buffer, 3, 1, 0, 0);
	vkCmdDrawIndexed(buffer, sizeof(testSquareIndices)/sizeof(uint32_t), 1, 0, 0, 0);

	vkCmdEndRenderPass(buffer);

	if(vkEndCommandBuffer(buffer) != VK_SUCCESS){
		throw std::runtime_error("could not end recording of cmd buffer");
	}
}	

VkShaderModule createShaderModule(std::vector<unsigned char> rawDataVector){
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(rawDataVector.data());
	createInfo.codeSize = rawDataVector.size();

	VkShaderModule returnModule;
	if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &returnModule) != VK_SUCCESS){
		throw std::runtime_error("\ncould not create shader module\n");
	}
	return returnModule;
}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height){
	VkCommandBuffer cmdBuffer = createSingleUseCmdBuffer();

	VkBufferImageCopy region{};
	region.bufferImageHeight = 0;//podría ser que en el bufer existieran padding bytes entre los renglones, pero no padding, así que 0 en este y rowLength
	region.bufferOffset = 0;//byte offset into the buffer
	region.bufferRowLength = 0;

	region.imageExtent = {width, height, 1};
	region.imageOffset = {0,0,0};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.mipLevel = 0;

	vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);

	endSingleUseCmdBuffer(cmdBuffer);
}

void createTextureImage(const char *imagePath, VkImage *image, VkDeviceMemory *imageMemory,  uint32_t desiredNumColChannels = STBI_rgb_alpha, VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB){
	//Pasar datos de imagen al stagingBuffer
	int imageLong, imageTall, numColChannels;
	stbi_uc *imageData = stbi_load(imagePath, &imageLong, &imageTall, &numColChannels, desiredNumColChannels);//estoy forzando a la imagen a ser cargada con 4 canales de color
	VkDeviceSize imageSize = imageLong * imageTall * 4;//El cuatro es el níumero de canales de color, podría ser numColChannels

	if(!imageData){
		throw std::runtime_error("could not load image data");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stagingBuffer, &stagingBufferMemory);

	void* data;
	
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, static_cast<uint32_t>(imageSize), 0, &data);
	memcpy(data, imageData, imageSize);
	vkUnmapMemory(logicalDevice, stagingBufferMemory);

	stbi_image_free(imageData);

	//Crear imagen
	VkImageCreateInfo imageCI{};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.arrayLayers = 1;
	imageCI.imageType = VK_IMAGE_TYPE_2D;//dice que tipo de coordenadas usará
	imageCI.extent.width = static_cast<uint32_t>(imageLong);
	imageCI.extent.height = static_cast<uint32_t>(imageTall);
	imageCI.extent.depth = 1;
	imageCI.mipLevels = 1;
	imageCI.format = imageFormat;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;//si fuera LINEAR ordenaria los pixeles como ps una matriz, OPTIMAL es óptimo, pero su ordenamiento es implementation defined
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;//UNDEFINED significa que le permitimos a la primera transición descartar pixeles
	imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.flags = 0;//Puede usarse para memory eficiency

	if(vkCreateImage(logicalDevice, &imageCI, nullptr, image) != VK_SUCCESS){
		throw std::runtime_error("could not create image");
	}

	//alojamos la memoria de la imágen como hariamos para un buffer
	VkMemoryRequirements imageMemoryRequirements;
	vkGetImageMemoryRequirements(logicalDevice, *image, &imageMemoryRequirements);

	VkMemoryAllocateInfo imageMemoryAI{};
	imageMemoryAI.allocationSize = imageMemoryRequirements.size;//alojo la cantidad que me dice que debo alojar, no el tamaño real
	imageMemoryAI.memoryTypeIndex = findRigthMemoryType(imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	imageMemoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	if(vkAllocateMemory(logicalDevice, &imageMemoryAI, nullptr, imageMemory) != VK_SUCCESS){
		throw std::runtime_error("Could not allocate image memory");
	}

	vkBindImageMemory(logicalDevice, *image, *imageMemory, 0);

	//Transferimos los datos del staging buffer a la imagen
	VkCommandBufferAllocateInfo commandBufferAI{};
	commandBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAI.commandPool = commandPool;
	commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAI.commandBufferCount = 1;

	VkCommandBuffer transferCommandBuffer;
	vkAllocateCommandBuffers(logicalDevice, &commandBufferAI, &transferCommandBuffer);
	
	//Antes de pasar los datos, tengo que transition el layout
	transitionImageLayout(*image, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);//primero a transfer dst optimal para meterle los datos
	copyBufferToImage(stagingBuffer, *image, static_cast<uint32_t>(imageLong), static_cast<uint32_t>(imageTall));
	transitionImageLayout(*image, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);//luego al óptimo para que lean de el

	//nos deshacemos del staging buffer
	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
}
//funciones necesarias para is Physical device suitable
bool checkValidationLayerSupport(){
	//conseguimos las layers que si puedo usar
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	
	std::vector<VkLayerProperties> supportedLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, supportedLayers.data());

	//checamos si las validation layers están en la lista de supported layers
	for (const char * layer : validationLayers){
		bool layerIsSupported = false;
		for (const auto& currSupportedLayer : supportedLayers){
			if (strcmp(layer, currSupportedLayer.layerName) == 0){
				layerIsSupported = true;
			}
		}
		if (!layerIsSupported){
			return false;
		}
	}
	return true;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device){
	uint32_t numExtension;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtension, nullptr);

	std::vector<VkExtensionProperties> supportedExtensions(numExtension);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtension, supportedExtensions.data());

	//checamos si las extensiones que necesitamos están en las extensiones que tienen soporte, podrías hacer 2 for loop, pero ps no
	std::set<std::string> requiredExtensions(usedExtensions.begin(), usedExtensions.end());

	for(const auto& extension : supportedExtensions){
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

bool isPhysicalDeviceSuitable(VkPhysicalDevice device){
	queueFamilyIndices selectedQueueFamilies = getSelectedQueueFamilies(device);
	//checamos extensiones, features y la swapChain
	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool featuresSupported = false;
	VkPhysicalDeviceFeatures physicalDeviceSupportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &physicalDeviceSupportedFeatures);

	if(physicalDeviceSupportedFeatures.samplerAnisotropy){
		featuresSupported = true;
	}

	bool swapChainHasAllRequirements = false;
	if (extensionsSupported){//debemos checar la swap chain solo si ya nos aseguramos que su extensión si tiene support
	
		SwapChainSupportDetails swapChainDetails = getSwapChainSupportDetails(device);
		swapChainHasAllRequirements = !swapChainDetails.formats.empty() && ! swapChainDetails.presentModes.empty();
	}

	return (selectedQueueFamilies.hasAllRequirements() && extensionsSupported && swapChainHasAllRequirements);//ponemos extensions supported por completness
}

void createDescriptorSetLayout(){//ESTA ES UNA DE ESAS FUNCIONES QUE SOLO SE HICIERON FUNCIONES PQ EL TUTORIAL QUIZO, NO PORQUE LO ESTEMOS REUTILIZANDO
	//especificamos el binding
	VkDescriptorSetLayoutBinding matrixBufferLayoutBinding{};
	matrixBufferLayoutBinding.binding = 0;
	matrixBufferLayoutBinding.descriptorCount = 1;//se suele hacer de varios, por ejemplos para dar la matrix de cada transformación de cada parte de un esqueleto
	matrixBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	matrixBufferLayoutBinding.pImmutableSamplers = nullptr;
	matrixBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;//se suele hacer de varios, por ejemplos para dar la matrix de cada transformación de cada parte de un esqueleto
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//todos los descriptor sets bindings se combinan en un VkDescriptorSetLayout

	VkDescriptorSetLayoutBinding allLayoutBindings[] = {matrixBufferLayoutBinding, samplerLayoutBinding};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
	descriptorSetLayoutCI.bindingCount = 2;
	descriptorSetLayoutCI.pBindings = allLayoutBindings;
	descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	
	if(vkCreateDescriptorSetLayout(logicalDevice, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout) != VK_SUCCESS){
		throw std::runtime_error("could not create descriptor set layout");
	}
}

void frameBufferResizeCallBack(GLFWwindow* ventanaCurr, int width, int heigth){
	windowResized = true;
}

void drawFrame(){
	//debemos sincronizar estas operaciones manualmente
	vkWaitForFences(logicalDevice, 1, &frameDrawnFences[currFrameDrawn], VK_FALSE, UINT64_MAX);

	//conseguir framebuffer: semaforo
	uint32_t imageIndex;
	VkResult resultRelatedToSwapChain = vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, gotframeBufferImageSemaforos[currFrameDrawn],VK_NULL_HANDLE, &imageIndex);
	if(resultRelatedToSwapChain == VK_ERROR_OUT_OF_DATE_KHR){
		remakeSwapChain();
		return;
	}else if(resultRelatedToSwapChain != VK_SUCCESS && resultRelatedToSwapChain != VK_SUBOPTIMAL_KHR){//im treating suboptimal as good enough
		throw std::runtime_error("could not get swapChain next image");
	}

	//we are here wich means we are actually working so:
	vkResetFences(logicalDevice, 1 ,&frameDrawnFences[currFrameDrawn]);

	//grabar comandos al frame: ocurre en cpu, no necesita sincronización
	vkResetCommandBuffer(commandBuffers[currFrameDrawn],0);//el segundo parámetro es una bitmask para flags
	grabarCommandBuffer(commandBuffers[currFrameDrawn], imageIndex);

	static int currTime = 0;
	currTime += 1;
	//actualizo las matrices//poner {} en matrixTransformations no las inicia como identidad
	MatrixTransformations matrixTransformations;//maybe all 3 matrices will change, view because of camera movement and proj because of window resize
	matrixTransformations.view = glm::lookAt(glm::vec3(0.0f,0.0f,1.0f),glm::vec3(0.0f,0.0f,0.0f),glm::vec3(0.0f,1.0f,0.0f));
	matrixTransformations.proj = glm::perspective(glm::radians(90.0f), chosenSwapExtent.width/(float)chosenSwapExtent.height, 0.1f, 10.0f);
	matrixTransformations.proj[1][1] *= -1;//cambiamos el signo de la Y porque en vulkan y crece hacia abajo
	matrixTransformations.model = glm::rotate(glm::mat4(1.0f), currTime * glm::radians(5.0f), glm::vec3(0.0f,1.0f,0.0f));
	memcpy(mappedUniformBufferMemories[currFrameDrawn], &matrixTransformations, sizeof(matrixTransformations));

	//submito el frame buffer: semaforo
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currFrameDrawn];

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &gotframeBufferImageSemaforos[currFrameDrawn];//Esperamos al semáforo 1 en la stage 1 de ambos array, en el semáforo 2 en la stage 2 y así
	VkPipelineStageFlags stagesToWaitIn[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};//espera en esta stage especificamente
	submitInfo.pWaitDstStageMask = stagesToWaitIn;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &imageWrittenSemaforos[currFrameDrawn];
	
	if (vkQueueSubmit(graphicsQueueHandle, 1, &submitInfo, frameDrawnFences[currFrameDrawn]) != VK_SUCCESS){
		throw std::runtime_error("could not submit command buffer");
	}

	//regresar la imagen a la swapChain
	VkPresentInfoKHR presentInfo{};
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pWaitSemaphores = &imageWrittenSemaforos[currFrameDrawn];
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.swapchainCount = 1;
	presentInfo.pResults = nullptr; //opcional, es un output para checar como le fué a cada swapChain individualmente

	resultRelatedToSwapChain = vkQueuePresentKHR(presentQueueHandle, &presentInfo);
	if(resultRelatedToSwapChain == VK_ERROR_OUT_OF_DATE_KHR || resultRelatedToSwapChain == VK_SUBOPTIMAL_KHR || windowResized){
		remakeSwapChain();
		windowResized = false;
	}else if(resultRelatedToSwapChain != VK_SUCCESS){
		throw std::runtime_error("error with queue present");
	}
	currFrameDrawn = (currFrameDrawn + 1) % MAX_FRAMES_DRAWN_AT_THE_SAME_TIME;//manera inteligente de loopear al inicio
}

int main() {
	printf("AAAAA");
	std::cout << "validation layers enabled: " << validationLayersEnabled << std::endl;
	/*window creation*/
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);//como es vulkan tenemos que hacer algo especial para rezizable windows
	ventana = glfwCreateWindow(SCREENWIDTH,SCREENHEIGTH,"uno",NULL,NULL);

	glfwSetFramebufferSizeCallback(ventana, frameBufferResizeCallBack);
	/*crear applicación*/
	// app info
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "App";
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.applicationVersion = VK_MAKE_API_VERSION(1,0,0,0);
	appInfo.engineVersion = VK_MAKE_API_VERSION(1,0,0,0);
	appInfo.pEngineName = "non";

	//InstanceInfo
	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;

	//glfw consigue las extensiones necesarias
	uint32_t glfwExtensionsCount = 0;
	const char ** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

	instanceInfo.enabledExtensionCount = glfwExtensionsCount;
	instanceInfo.ppEnabledExtensionNames = glfwExtensions;

	//le damos las validation layers
	if (validationLayersEnabled){
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceInfo.ppEnabledLayerNames = validationLayers.data();
	}else{
		instanceInfo.enabledLayerCount = 0;
	}

	//ya creamos la instance,
	VkInstance vulkanInstance;

	if(validationLayersEnabled && !checkValidationLayerSupport()){
		throw std::runtime_error("one or more validation layers are not supported");
	}else{
		printf("\nno problem with validation layers\n");
	}

	VkResult resultado = vkCreateInstance(&instanceInfo, nullptr, &vulkanInstance);

	if (resultado == VK_SUCCESS){
		printf("AAAAAAAAAAAAAAAAA");
	}else{
		printf("NOOOOOOOOOOOOOOOOOOOOO");
	}

	/*Creamos window surface*/ //debe ser creada justo después de la instance porque influencia la decisión de physical device
	if (glfwCreateWindowSurface(vulkanInstance, ventana, nullptr, &windowSurface) != VK_SUCCESS){//glfw nos ayuda a saltarnos 30000000 lineas
		throw std::runtime_error("could not create window surface");
	}else{
		printf("\nwindow surface created\n");
	}

	/*elegimos la targeta gráfica (physical device)*/
	//conseguimos lista de devices
	uint32_t ndevices = 0;
	vkEnumeratePhysicalDevices(vulkanInstance, &ndevices, nullptr);

	if (ndevices == 0){
		throw std::runtime_error("old as fuck computer");}

	std::vector<VkPhysicalDevice> allDevices(ndevices);
	vkEnumeratePhysicalDevices(vulkanInstance, &ndevices, allDevices.data());

	//buscamos un device que nos sirva
	for (const auto& currDevice : allDevices){
		if (isPhysicalDeviceSuitable(currDevice)){
			physicalDevice = currDevice;
			break;//podriamos no poner este break si quieres obtener todos los devices que son compatibles
		}
	}

	if (physicalDevice == VK_NULL_HANDLE){
		throw std::runtime_error("could not pick a device, you have no decent GPUs");
	}else{
		printf("\nappropiate device found\n");
	}
	
	/*hacemos logical device*/
	//especificamos las queues que se crearán
	queueFamilyIndices selectedQueueFamilies = getSelectedQueueFamilies(physicalDevice);
	float allQueuePriority = 1.0f;//todas van a tener la misma prioridad de momento

	//hacemos la createInfo de cada queue
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(0);
	std::set<uint32_t> uniqueQueueIndices(selectedQueueFamilies.allFamilyIndices,selectedQueueFamilies.allFamilyIndices + sizeof(selectedQueueFamilies.allFamilyIndices)/sizeof(uint32_t));
	for(uint32_t familyIndex : uniqueQueueIndices){
		VkDeviceQueueCreateInfo queueCreateInfo{};

		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pQueuePriorities = &allQueuePriority;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = familyIndex;
		
		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	
	//especificamos las features que se usarán
	VkPhysicalDeviceFeatures deviceFeaturesUsed{};//ninguna en especial así que dejamos todo en VK_FALSE
	deviceFeaturesUsed.samplerAnisotropy = VK_TRUE;//Character development
	printf("\nsize of queueCreateInfos vector: %u\n", static_cast<uint32_t>(uniqueQueueIndices.size()));
	///hacemos createinfo
	VkDeviceCreateInfo logicalDeviceCreateInfo{};
	logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(uniqueQueueIndices.size());
	logicalDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	logicalDeviceCreateInfo.pEnabledFeatures = &deviceFeaturesUsed;

	//Las siguientes están deprecadas?, pero para compatibilidad se pueden poner
	logicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(usedExtensions.size());
	logicalDeviceCreateInfo.ppEnabledExtensionNames = usedExtensions.data();
	if (validationLayersEnabled){
		logicalDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		logicalDeviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}else{
		logicalDeviceCreateInfo.enabledLayerCount = 0;
	}
	
	//lo creamos
	if (vkCreateDevice(physicalDevice, &logicalDeviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS){
		throw std::runtime_error("could not create logical device");
	}else{
		printf("\nlogical device created\n");
	}	

	//conseguimos las queueHandles que se hicieron al mismo tiempo que el logical device
	vkGetDeviceQueue(logicalDevice, selectedQueueFamilies.graphicsFamilyIndex, 0, &graphicsQueueHandle); // en queue index va su indice de queue de esta familia, solo tenemos uno de cada familia así que es 0 en todos.
	vkGetDeviceQueue(logicalDevice, selectedQueueFamilies.presentFamilyIndex, 0, &presentQueueHandle); 
	vkGetDeviceQueue(logicalDevice, selectedQueueFamilies.transferFamilyIndex, 0, &transferQueueHandle);

	/*creamos la swap chain*///esto también obtiene las handles a sus imágenes
	createSwapChain();
	VkSurfaceFormatKHR chosenSwapSurfaceFormat = getSwapChainSurfaceFormat(getSwapChainSupportDetails(physicalDevice));
	
	/*image views*/
	createImageViews();	

	//checar las extensiones que tenemos ps nomás
	uint32_t extCount = 0;

	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	std::vector<VkExtensionProperties> extProperties(extCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extProperties.data());

	//printf("\n\nExtensiones\n");
	for (int i = 0; i < extCount; i++){
	//	printf("\n\t %s \n", extProperties[i].extensionName);
	}

	//craemos el descriptor set layout que se necesita para la pipeline
	createDescriptorSetLayout();

	/*graphics pipeline*/
	//read program data
	std::vector<unsigned char> testVertexData = readRawBinary("./testProgram.vert.spv");
	std::cout << "\ntest fragment data size: " << testVertexData.size() << std::endl;

	std::vector<unsigned char> testFragmentData = readRawBinary("./testProgram.frag.spv");
	std::cout << "\ntest vertex data size: " << testFragmentData.size() << std::endl;

	std::vector<unsigned char> vertexData = readRawBinary("./bufferShaderProgram.vert.spv");
	std::cout << "\nfragment data size: " << vertexData.size() << std::endl;

	std::vector<unsigned char> fragmentData = readRawBinary("./bufferShaderProgram.frag.spv");
	std::cout << "\nvertex data size: " << fragmentData.size() << std::endl;

	//create shader modules
	VkShaderModule vertexShaderModule = createShaderModule(vertexData);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentData);

	//create shader stages
	VkPipelineShaderStageCreateInfo vertexStageCreateinfo{};
	vertexStageCreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexStageCreateinfo.module = vertexShaderModule;
	vertexStageCreateinfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexStageCreateinfo.pName = "main";//nombre del entrypoint

	VkPipelineShaderStageCreateInfo fragmentStageCreateInfo{};
	fragmentStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentStageCreateInfo.module = fragmentShaderModule;
	fragmentStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentStageCreateInfo.pName = "main";//nombre del entrypoint

	VkPipelineShaderStageCreateInfo shaderStagesCreateInfo[] = {vertexStageCreateinfo, fragmentStageCreateInfo};

	//dynamic state, lo que si se puede cambiar sin recrear toda la pipeline
	std::vector<VkDynamicState> dynamicState = {VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicState.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicState.data();
	
	//Vertex input state, decimos en que formato le damos los vertices
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	//binding descriptors
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;//indice de este binding en el array de bindings
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;//cada chunk corresponde a un vertice
	bindingDescription.stride = sizeof(Vertex);

	//Attribute descriptions
	VkVertexInputAttributeDescription attributeDescriptions[3]{};
	attributeDescriptions[0].binding = 0;//el mismo que su binding descriptor
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].offset = offsetof(Vertex, imgPos);

	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 3;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions;

	//depth stencil state
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
	depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilStateCI.depthTestEnable = VK_TRUE;
	depthStencilStateCI.depthWriteEnable = VK_TRUE;
	
	depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;//solo quedarse con los fragmentos que estan entre maxDetphBounds y minDepthBounds
	depthStencilStateCI.maxDepthBounds = 1.0f;
	depthStencilStateCI.minDepthBounds = 0.0f;
	depthStencilStateCI.stencilTestEnable = VK_FALSE;
	depthStencilStateCI.back = {};//estos son del stencil
	depthStencilStateCI.front = {};

	//Input assembly, que va a dibujar con este input y como
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	
	//viewport, la imagen se estira o comprime para tener estas dimensiones
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.height = (float)chosenSwapExtent.height;//the swapChain imgs could differ from SCREENSIZE, so we use that
	viewport.width = (float)chosenSwapExtent.width;
	viewport.minDepth = 0.0f;//must be between [0.0,1.0]
	viewport.maxDepth = 1.0f;//can be anything

	//scissors, lo que este fuera se descarta
	VkRect2D tijeras{};
	tijeras.offset = {0, 0};
	tijeras.extent = chosenSwapExtent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.viewportCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo{};
	rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationCreateInfo.depthClampEnable = VK_FALSE;//clampea lo que se sale del frustum en vez de cortarlo
	rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;//la geometria nunca deja el resterizador
	rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationCreateInfo.lineWidth = 1.0f;//medido en fragmentos
	rasterizationCreateInfo.cullMode = VK_CULL_MODE_NONE; //VK_CULL_MODE_BACK_BIT;//
	rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

	rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationCreateInfo.depthBiasClamp = 0.0f;
	rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;

	//multisampling
	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{};
	multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleCreateInfo.alphaToOneEnable = VK_FALSE;
	multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleCreateInfo.minSampleShading = 1.0f;
	multisampleCreateInfo.pSampleMask = nullptr;
	multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	//Color blending, necesite global y perframebuffer
	VkPipelineColorBlendAttachmentState blendAttachmentState{};//para añadir transparencia, solo adivinas los enums basado en como quieres que los combine
	blendAttachmentState.blendEnable = VK_FALSE;
	blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |VK_COLOR_COMPONENT_G_BIT| 
										  VK_COLOR_COMPONENT_B_BIT| VK_COLOR_COMPONENT_A_BIT;
	blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;

	VkPipelineColorBlendStateCreateInfo blendCreateInfo{};
	blendCreateInfo.attachmentCount = 1;
	blendCreateInfo.logicOpEnable = VK_FALSE;//tambien podrías blendear con bitwise ops, pero poner esto true hace el attachment false
	blendCreateInfo.pAttachments = &blendAttachmentState;
	blendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	blendCreateInfo.blendConstants[0] = 0.0f;
	blendCreateInfo.blendConstants[1] = 0.0f;
	blendCreateInfo.blendConstants[2] = 0.0f;
	blendCreateInfo.blendConstants[3] = 0.0f;
	
	//Pipeline layout, maneja las uniform
	VkPipelineLayoutCreateInfo pipelineLayoutCI{};
	pipelineLayoutCI.pPushConstantRanges = nullptr;
	pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutCI.pushConstantRangeCount = 0;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	if(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCI, nullptr, &pipelineLayout) != VK_SUCCESS){
		throw std::runtime_error("could not create pipeline");
	}

	/*render pass*/
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = chosenSwapSurfaceFormat.format;//debe ser el mismo pq ps ese usamos
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;//no multisampling
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;//what to do before and after rendering
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//subpasses, juntar todos los efectos de post procesamiento en una pass usando subpasses le permite al gpu o a vulkan hacer optimizaciones
	//cada subpass referencia un attachment
	VkAttachmentReference colorAttachmentReference{};
	colorAttachmentReference.attachment = 0;//indice en el array de attachments
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;//el layout con el que trataré ese attachment, en este caso como color buffer

	//las de depth buffer	
	VkFormat supportedDepthBufferFormat = findSupportedFormat({VK_FORMAT_D32_SFLOAT_S8_UINT,VK_FORMAT_D24_UNORM_S8_UINT},VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = supportedDepthBufferFormat;//debe ser el mismo pq ps ese usamos
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;//no multisampling
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;//what to do before and after rendering
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;//aqui nos vale vrg a diferencia del color attachment
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference{};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	//cada subpass necesita descripción
	VkSubpassDescription subpassDescription{};//la subpass de color y de depth es la misma
	subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;//esta subpass es de graficos
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;//

	VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};//POSSIBLE BUG SOURCE
	//creamos renderPass
	VkRenderPassCreateInfo renderPassCI{};
	renderPassCI.attachmentCount = 2;
	renderPassCI.pAttachments = attachments;
	renderPassCI.pSubpasses = &subpassDescription;
	renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCI.subpassCount = 1;

	//subpass dependencies, lidian con trnasitions (especifica memory y execution dependencies entre subpasses)
	//tenemos 3 subpasses, la que creamos, la operación antes y la operación después, vulkan tiene built-in dependencies que lidian con ellas pero hay que sincronizar la de la operación después
	VkSubpassDependency subpassDependency{};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL; // VK_SUBPASS_EXTERNAL se refiere a la operación antes o después dependiendo de si está en .srcSubpass o .dstSubpass
	subpassDependency.dstSubpass = 0;//indice de subpass, en este caso la de color
	//Esperamos a:
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;//esperaremos a esta operación
	subpassDependency.srcAccessMask = 0;//específicamente a que 0 termine, osea a que 

	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;//esta operación será la que espere
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;//específicamente esperaremos hasta que acabe y luego escribiremos

	renderPassCI.dependencyCount = 1;
	renderPassCI.pDependencies = &subpassDependency;

	if(vkCreateRenderPass(logicalDevice, &renderPassCI, nullptr, &renderPass) != VK_SUCCESS){
		throw std::runtime_error("could not create render pass");
	}

	//ya crear la pipeline
	VkGraphicsPipelineCreateInfo graphicsPipelineCI{};
	graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;//opcional, es para crear subpipelines que es más rapido que varias pipelines
	graphicsPipelineCI.basePipelineIndex = -1;//opcional, tiene que ver con parametro anterior
	graphicsPipelineCI.layout = pipelineLayout;
	graphicsPipelineCI.renderPass = renderPass;
	graphicsPipelineCI.stageCount = 2;
	graphicsPipelineCI.pStages = shaderStagesCreateInfo;
	graphicsPipelineCI.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCI.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsPipelineCI.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCI.pColorBlendState = &blendCreateInfo;
	graphicsPipelineCI.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCI.pDepthStencilState = &depthStencilStateCI;
	graphicsPipelineCI.pMultisampleState = &multisampleCreateInfo;
	graphicsPipelineCI.pRasterizationState = &rasterizationCreateInfo;
	graphicsPipelineCI.subpass = 0;//indice

	if(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, &graphicsPipeline) != VK_SUCCESS){
		throw std::runtime_error("could not create graphics pipeline");
	}

	//Destroy shader modules as soon as the code is in te pipeline just like openGL
	vkDestroyShaderModule(logicalDevice, vertexShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragmentShaderModule, nullptr);

	/*command buffers*/
	//deben estar en command pools, que manejan su memoria 
	VkCommandPoolCreateInfo commandPoolCI{};
	commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCI.queueFamilyIndex = selectedQueueFamilies.graphicsFamilyIndex;
	commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//le digo que me deje actualizarlos por separado

	if(vkCreateCommandPool(logicalDevice, &commandPoolCI, nullptr, &commandPool) != VK_SUCCESS){
		throw std::runtime_error("could not create command pool");
	}

	//de paso creamos la de transfer 
	VkCommandPoolCreateInfo transferCommandPoolCI{};
	transferCommandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferCommandPoolCI.queueFamilyIndex = selectedQueueFamilies.transferFamilyIndex;
	transferCommandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//le digo que me deje actualizarlos por separado

	if(vkCreateCommandPool(logicalDevice, &transferCommandPoolCI, nullptr, &transferCommandPool) != VK_SUCCESS){
		throw std::runtime_error("could not create transfer command pool");
	}

	//alojamos la memoria para los buffers//automáticamente desalojados al destruir su pool
	VkCommandBufferAllocateInfo commandBufferAlocateInfo{};
	commandBufferAlocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAlocateInfo.commandPool = commandPool;
	commandBufferAlocateInfo.commandBufferCount = MAX_FRAMES_DRAWN_AT_THE_SAME_TIME;//se alojan varios a la vez usualmente
	commandBufferAlocateInfo.level =VK_COMMAND_BUFFER_LEVEL_PRIMARY;//esta primario (se da directamente a la queue) y secundario (lo llama un command buffer primario), esto para reusar operaciones comunes

	if(vkAllocateCommandBuffers(logicalDevice, &commandBufferAlocateInfo, commandBuffers.data()) != VK_SUCCESS){
		throw std::runtime_error("could not create command buffer");
	}

	//creamos el vertexBuffer con una funcion, debe estar debajo de la creación de command pool
	createVertexBuffer();
	createIndexBuffer();

	/*creamos primitivos de sincronización*/
	for (int i = 0; i < MAX_FRAMES_DRAWN_AT_THE_SAME_TIME; i++){
		
		VkFenceCreateInfo fenceCI{};//de echo estos primitivos no tienen parámetros, esto es para forward compatibility
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;//ps pa que en el primer frame diga inmediatamente que ya terminó de dibujar el anterior

		VkSemaphoreCreateInfo semaforoCI{};
		semaforoCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if(vkCreateFence(logicalDevice, &fenceCI, nullptr, &frameDrawnFences[i]) != VK_SUCCESS
		|| vkCreateSemaphore(logicalDevice, &semaforoCI, nullptr, &gotframeBufferImageSemaforos[i]) != VK_SUCCESS
		|| vkCreateSemaphore(logicalDevice, &semaforoCI, nullptr, &imageWrittenSemaforos[i]) != VK_SUCCESS){
			throw std::runtime_error("could not create sync primitives");
		}
	}

	/*uniform buffers*/
	for (int i = 0; i < MAX_FRAMES_DRAWN_AT_THE_SAME_TIME; i++){
		createBuffer(sizeof(MatrixTransformations), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					&uniformBuffers[i], &uniformBufferMemories[i]);
		
		vkMapMemory(logicalDevice, uniformBufferMemories[i], 0, sizeof(MatrixTransformations), 0, &mappedUniformBufferMemories[i]);
	}

	/*descriptor sets*///para las uniforms necesito algo que apunte a su localización (descriptor), algo que diga como leer esa información (descriptor layout), y todos los descriptor se ponen en un set de descriptores, los cuales se crean desde pools como los commandBuffers
	VkDescriptorPoolSize descriptorPoolSize{};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_DRAWN_AT_THE_SAME_TIME);//max descriptor (not sets)

	VkDescriptorPoolSize samplerPoolSize{};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_DRAWN_AT_THE_SAME_TIME);//como está en set con las uniform, voy a necesitar repetir

	VkDescriptorPoolSize allDescriptorPoolSizes[] = {descriptorPoolSize, samplerPoolSize};
	//No haces la pool lo suficientemente grande es uno de los problemas que las validation layers de vulkan 1.1 no atrapan, regresan POOL OUT OF MEMORY
	VkDescriptorPoolCreateInfo descriptorPoolCI{};
	descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCI.pPoolSizes = allDescriptorPoolSizes;
	descriptorPoolCI.poolSizeCount = 2;
	descriptorPoolCI.maxSets = static_cast<uint32_t>(MAX_FRAMES_DRAWN_AT_THE_SAME_TIME);

	if(vkCreateDescriptorPool(logicalDevice, &descriptorPoolCI, nullptr, &descriptorPool) != VK_SUCCESS){
		throw std::runtime_error("could not crate descriptor pool");
	}
	
	//ahora que tengo la pool, puedo crearlo
	VkDescriptorSetAllocateInfo descriptorSetAI{};
	descriptorSetAI.descriptorPool = descriptorPool;
	descriptorSetAI.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_DRAWN_AT_THE_SAME_TIME);
	descriptorSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_DRAWN_AT_THE_SAME_TIME, descriptorSetLayout);//vector de longitud 2 con todos los valores inicializados con descriptorSetLayout
	descriptorSetAI.pSetLayouts = descriptorSetLayouts.data();
	
	if (vkAllocateDescriptorSets(logicalDevice, &descriptorSetAI, descriptorSets.data()) != VK_SUCCESS){
		throw std::runtime_error("could not allocate descriptor sets");
	}

	/*imágenes*///como las voy a meter al mismo descriptor set las pongo aquí
	//la creamos con una función
	createTextureImage("./testImage.jpg", &testImage, &testImageMemory);
	
	//creamos la image view
	VkImageViewCreateInfo testImageViewCI{};
	testImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	testImageViewCI.image = testImage;

	testImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	testImageViewCI.format = VK_FORMAT_R8G8B8A8_SRGB;
	//estos te permiten mucha customización de los color channels
	testImageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	testImageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	testImageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	testImageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	testImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;//indicamos que esta imágen es color target
	testImageViewCI.subresourceRange.baseArrayLayer = 0;// no queremos mip maps ni multiple layers
	testImageViewCI.subresourceRange.baseMipLevel = 0;
	testImageViewCI.subresourceRange.layerCount = 1;
	testImageViewCI.subresourceRange.levelCount = 1;

	VkImageView testImageView;
	if(vkCreateImageView(logicalDevice, &testImageViewCI, nullptr, &testImageView) != VK_SUCCESS){
		throw std::runtime_error("could not create test image image view");
	}

	//Sampler
	VkSamplerCreateInfo testSamplerCI{};
	testSamplerCI.unnormalizedCoordinates = VK_FALSE;//para que use [0-1) en vez de [0,imageLong)
	testSamplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	testSamplerCI.compareEnable = VK_FALSE;
	testSamplerCI.addressModeU =VK_SAMPLER_ADDRESS_MODE_REPEAT;
	testSamplerCI.addressModeV =VK_SAMPLER_ADDRESS_MODE_REPEAT;
	testSamplerCI.addressModeW =VK_SAMPLER_ADDRESS_MODE_REPEAT;
	testSamplerCI.magFilter = VK_FILTER_LINEAR;//para que no se vea blocky
	testSamplerCI.minFilter = VK_FILTER_LINEAR;

	//necesitamos saber cuanto de anisotropy soporta el physical device
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	testSamplerCI.anisotropyEnable = VK_TRUE;
	testSamplerCI.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;

	testSamplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;//solo se usa com clamp to border
	testSamplerCI.compareOp = VK_COMPARE_OP_ALWAYS;

	testSamplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;//estos 4 son de mipmaping
	testSamplerCI.mipLodBias = 0.0f;
	testSamplerCI.minLod = 0.0f;
	testSamplerCI.maxLod = 0.0f;
	VkSampler testSampler;
	if(vkCreateSampler(logicalDevice, &testSamplerCI, nullptr, &testSampler) != VK_SUCCESS){
		throw std::runtime_error("could not create test sampler");
	}

	//ahora que los alojé ya
	for(size_t i = 0; i < MAX_FRAMES_DRAWN_AT_THE_SAME_TIME; i++){
		VkDescriptorBufferInfo currBI{};
		currBI.buffer = uniformBuffers[i];
		currBI.offset = 0;
		currBI.range = sizeof(MatrixTransformations);

		VkDescriptorImageInfo currII{};//tengo el sampler y la textura en el descriptor set de los uniforms, ndmas en otro binding
		currII.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;//en este lo tengo en createTextureImage
		currII.imageView = testImageView;
		currII.sampler = testSampler;

		std::array<VkWriteDescriptorSet, 2>currWriteDescriptorSets{};//No funcionaba con arrays de c???
		currWriteDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		currWriteDescriptorSets[0].dstSet = descriptorSets[i];
		currWriteDescriptorSets[0].dstBinding = 0;//el del shader
		currWriteDescriptorSets[0].dstArrayElement = 0;//los descriptores pueden ser arrays, este es el primer indice en el array que queremos actualizar
		currWriteDescriptorSets[0].descriptorCount = 1;//enmpezamos en 0 (dstArrayElement) y actualizamos 1
		currWriteDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		//solo llenas el tipo de descriptor estes usando
		currWriteDescriptorSets[0].pBufferInfo = &currBI;//para buffers
		currWriteDescriptorSets[0].pImageInfo = nullptr;//para image data
		currWriteDescriptorSets[0].pTexelBufferView = nullptr;//para bufferviews

		currWriteDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		currWriteDescriptorSets[1].descriptorCount = 1;
		currWriteDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		currWriteDescriptorSets[1].dstArrayElement = 0;
		currWriteDescriptorSets[1].dstSet = descriptorSets[i];
		currWriteDescriptorSets[1].dstBinding = 1;

		currWriteDescriptorSets[1].pImageInfo = &currII;//no es necesario poner los otros en nullptr?
		currWriteDescriptorSets[1].pBufferInfo = nullptr;
		currWriteDescriptorSets[1].pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(logicalDevice, 2, currWriteDescriptorSets.data(), 0, nullptr);// lo de copy descriptors es ps para copiarlos
	}

	/*depth buffer*/
	//imagen
	VkImageCreateInfo depthImageCI{};
	depthImageCI.arrayLayers = 1;
	depthImageCI.extent = {chosenSwapExtent.width, chosenSwapExtent.height, 1};
	depthImageCI.flags = 0;//Opcional par amemory eficiency
	depthImageCI.format = supportedDepthBufferFormat;
	depthImageCI.imageType = VK_IMAGE_TYPE_2D;
	depthImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthImageCI.mipLevels = 1;
	depthImageCI.pQueueFamilyIndices = nullptr;//Optional
	depthImageCI.queueFamilyIndexCount = 0;
	depthImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	depthImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	depthImageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depthImageCI.tiling = 
	VK_IMAGE_TILING_OPTIMAL;
	depthImageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	
	if( vkCreateImage(logicalDevice, &depthImageCI, nullptr, &depthBuffer) != VK_SUCCESS){
		throw std::runtime_error("could not create depth buffer image");
	}

	//memory
	VkMemoryRequirements depthBufferMemoryRequirements;
	vkGetImageMemoryRequirements(logicalDevice, depthBuffer, &depthBufferMemoryRequirements);

	VkMemoryAllocateInfo depthBufferMemoryAI{};
	depthBufferMemoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	depthBufferMemoryAI.allocationSize = depthBufferMemoryRequirements.size;
	depthBufferMemoryAI.memoryTypeIndex = findRigthMemoryType(depthBufferMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	vkAllocateMemory(logicalDevice, &depthBufferMemoryAI, nullptr, &depthBufferMemory);

	vkBindImageMemory(logicalDevice, depthBuffer, depthBufferMemory, 0);

	//image view
	//necesito checar si el formato que quiero tiene support de depth y stencil

	VkImageViewCreateInfo depthBufferImageViewCI{};
	depthBufferImageViewCI.format = supportedDepthBufferFormat;
	depthBufferImageViewCI.image = depthBuffer;
	depthBufferImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

	depthBufferImageViewCI.subresourceRange.baseArrayLayer = 0;
	depthBufferImageViewCI.subresourceRange.baseMipLevel = 0;
	depthBufferImageViewCI.subresourceRange.layerCount = 1;
	depthBufferImageViewCI.subresourceRange.levelCount = 1;
	depthBufferImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

	depthBufferImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthBufferImageViewCI.flags = 0;
	vkCreateImageView(logicalDevice, &depthBufferImageViewCI,nullptr, &depthBufferImageView);

	//ahora que ya creé los recursos para el depth buffer debo de cambiar su layout
	transitionImageLayout(depthBuffer, supportedDepthBufferFormat,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	/*framebuffer, son todos los attachment (ej: color)*/
	createFrameBuffers();//debo moverlo hasta después de que cree los recursos del depthBuffer
	/*main loop*/
	while (!glfwWindowShouldClose(ventana)) {
		glfwPollEvents();
		drawFrame();
	}
	vkDeviceWaitIdle(logicalDevice);


	for (auto familyIndex : selectedQueueFamilies.allFamilyIndices){
		printf("curr family: %u\n", familyIndex);
	}
	printf("are graphics and present family equal: %d", presentQueueHandle == graphicsQueueHandle);

	//cleanup
	vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);

	for (int i = 0; i < MAX_FRAMES_DRAWN_AT_THE_SAME_TIME; i++){
		vkDestroyBuffer(logicalDevice, uniformBuffers[i], nullptr);
		vkFreeMemory(logicalDevice, uniformBufferMemories[i], nullptr);
	}

	for (int i = 0; i < MAX_FRAMES_DRAWN_AT_THE_SAME_TIME; i++){
		vkDestroyFence(logicalDevice, frameDrawnFences[i], nullptr);
		vkDestroySemaphore(logicalDevice, imageWrittenSemaforos[i], nullptr);
		vkDestroySemaphore(logicalDevice, gotframeBufferImageSemaforos[i], nullptr);
	}
	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
	vkDestroyCommandPool(logicalDevice, transferCommandPool, nullptr);
	
	destroySwapChain();//frameBuffers se deben destruir antes de render pass e image views

	vkDestroyImage(logicalDevice, testImage, nullptr);
	vkFreeMemory(logicalDevice, testImageMemory, nullptr);
	vkDestroyImageView(logicalDevice, testImageView, nullptr);
	vkDestroySampler(logicalDevice, testSampler, nullptr);

	vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);
	vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
	vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);
	vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
	vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
	vkDestroySurfaceKHR(vulkanInstance, windowSurface, nullptr);//necesita destruirse antes de su instance
	vkDestroyDevice(logicalDevice, nullptr); //ESTO DEBE DE ESTAR ANTES DE DESTROY INSTANCE
	vkDestroyInstance(vulkanInstance, nullptr);//we destroy the instance for cleanup and avoiding leaks

	glfwDestroyWindow(ventana);
	glfwTerminate();
	
	return 0;
}

/*SEMAFOROS
	estan lo semáforos binario y los de timeline, solo usaremos los binarios

	al iniciar una operación, le damos un semáforo: 
	-si lo ponemos como signal, se pondrá en verde cuando termina la operación
	-si lo ponemos como wait, la operación no iniciará hasta que el semáforo esté en verde

	En cuando el que lo tenga como wait inicie, el semáforo se resetea y está listo para otro uso
	*/
	/*FENCES
	este se usa para sincronizar el cpu en vez de gpu

	al iniciar una operación le damos la fence:
	-al proceso para que la ponga en verde cuando termine
	-llamamos una función que hará al cpu esperar a que la fence esté en verde

	Estas deben ser reseteadas manualmente
	En general se prefieren semáforors pq detener al cpu no es ideal
	*/
	
	//Lo regreso a la swapChain

	//Esperar a que el anterior frame termine: fence
	//dibujar varios frames en vez de esperar a que se termine de dibujar el anterior
	//si quiero dibujar más de uno necesito:
		//no pasarme de las imágenes de la swapchain
			//espero a que la cantidad de imágenes usadas sea menor a la cantidad de imágenes en la swapchain
			//luego pido otro frame
		//no resetear frameBuffers que se estén usando
			//tener la misma cantidad de buffers que imágenes?
			//keep track of frames drawn
			//el commandBuffer que reseteo sería commandBuffers[framesBeingDrawn - 1]
		//que cada writeToBuffer espere a que su imagen haya sido conseguida
			//semáforo para cada imagen?
			//lo mismo que command buffer?


	//si no esperara:
		//obtendría la siguiente imagen
			//eventualmente se me acabarían las imágenes de la swapChain*
		//resetearía el commandBuffer
			//el commandBuffer se resetearía mientras se está usando*
		//esperaría a que el semáforo me de greenligth para conseguir imagen
			//todos los frames avanzarían en cuanto uno de ellos diera greenligth
		//iría de que wowowowow lopeando por drawFrame cuando no pudiera ejecutar
	
	