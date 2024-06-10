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

//local
#include <ASG_utils.hpp>
#include <ASG_vertex.hpp>
#include <ASG_swapChain.hpp>
#include <ASG_graphicsPipeline.hpp>
#include <ASGraphics.hpp>

#include <dependencies/binFileLoader.hpp>

/*definitions echas por mi*/
#define SCREENWIDTH 500
#define SCREENHEIGTH 500
#define MAX_FRAMES_IN_FLIGHT (uint32_t)2

/*globales*/
VkInstance vulkanInstance;

std::unique_ptr<asgSwapChain> swapChain;
std::vector<VkFramebuffer> frameBuffers;

std::unique_ptr<asgPipeline> graphicsPipeline;

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

std::vector<VkCommandBuffer> commandBuffers(MAX_FRAMES_IN_FLIGHT);

VkDescriptorPool descriptorPool;
std::vector<VkBuffer> uniformBuffers(MAX_FRAMES_IN_FLIGHT);

std::vector<VkDeviceMemory> uniformBufferMemories(MAX_FRAMES_IN_FLIGHT);//TODO use arrays instead?
std::vector<void*> mappedUniformBufferMemories(MAX_FRAMES_IN_FLIGHT);
std::vector<VkDescriptorSet> descriptorSets(MAX_FRAMES_IN_FLIGHT);

bool windowResized = false;

std::vector<VkSemaphore> gotframeBufferImageSemaforos(MAX_FRAMES_IN_FLIGHT);
std::vector<VkSemaphore>imageWrittenSemaforos(MAX_FRAMES_IN_FLIGHT);
std::vector<VkFence> frameDrawnFences(MAX_FRAMES_IN_FLIGHT);
uint32_t currFrameDrawn = 0;
const std::vector<const char*> usedExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};//es un typedef string
const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

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

/////*functions*////////

/*declarations*/
//helpers
bool isPhysicalDeviceSuitable(VkPhysicalDevice device);
bool checkValidationLayerSupport();
bool checkDeviceExtensionSupport(VkPhysicalDevice device);
void createFrameBuffers();
void destroyFrameBuffers();

/*definitions*/
void createFrameBuffers() {
	frameBuffers.resize(swapChain->images.size());
	for (size_t i = 0; i < swapChain->views.size(); i++) {//creamos framebuffer de cada color attachment  
		VkImageView currAttachments[] = { swapChain->views[i], swapChain->depthBufferImageView };//solo es el de color pero ps luego le meteremos mas attachments //podemos usar el mismo depthBuffer porque solo una subpass ocurre simultaneamente debido a nuestros semáforos
		VkFramebufferCreateInfo currCI{};
		currCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		currCI.renderPass = graphicsPipeline->renderPass; //decimos que debe ser compatible con esta render pass
		currCI.attachmentCount = 2;
		currCI.pAttachments = currAttachments;
		currCI.width = swapChain->swapExtent.width;
		currCI.height = swapChain->swapExtent.height;
		currCI.layers = 1;

		if (vkCreateFramebuffer(logicalDevice, &currCI, nullptr, &(frameBuffers[i])) != VK_SUCCESS) {
			throw std::runtime_error("could not create frame buffer");
		}
	}
}
void destroyFrameBuffers() {
	for (auto currFrameBuffer : frameBuffers) {//debemos destruirlo antes de la render pass e image views
		vkDestroyFramebuffer(logicalDevice, currFrameBuffer, nullptr);
	}
}

//Buffer functions
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

	copyBuffer(stagingBuffer, 0, indexBuffer, 0, sizeof(testSquareIndices));

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
	copyBuffer(stagingBuffer, 0, vertexBuffer, 0, sizeof(testSquare));

	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	
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

void remakeSwapChain(){
	waitUntilCanRemakeSwapChain();
	destroyFrameBuffers();
	swapChain = std::make_unique<asgSwapChain>();
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
	renderPassInfo.renderPass = graphicsPipeline->renderPass;
	renderPassInfo.framebuffer = frameBuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0,0};
	renderPassInfo.renderArea.extent = swapChain->swapExtent;

	//clear value
	VkClearValue clearValues[2];//EL ORDEN DE ESTOS CLEAR VALUES DEBE SER IDENTICO AL DE LOS ATTACHMENTS
	clearValues[0].color = {0.0f,0.0f,0.0f};//POSIBLE BUG SOURCE PQ NO INICALIZO LOS STRUCTS
	clearValues[1].depthStencil = {1.0f, 0};//los pongo todos en 1.0f, el valor de profundidad más lejano, el 0 es stencil

	renderPassInfo.clearValueCount = 2;
	renderPassInfo. pClearValues = clearValues;

	//iniciar la render pass
	vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);//3er argumento, los comandos de la render pass serán embedded en el buffer sin usar un buffer secundario
	vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->handle);

	VkBuffer buffers[] = {vertexBuffer};
	VkDeviceSize bufferOffsets[] = {0};
	vkCmdBindVertexBuffers(buffer, 0, 1, buffers, bufferOffsets);
	vkCmdBindIndexBuffer(buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->pipelineLayout, 0, 1, &descriptorSets[currFrameDrawn], 0, nullptr);//tengo que especificar si a la graphics o compute pipeline
	//ViewPort y Scissors, pq los pusimos como dynamic state
	VkViewport viewport{};
	viewport.height = static_cast<float>(swapChain->swapExtent.height);
	viewport.width = static_cast<float>(swapChain->swapExtent.width);
	viewport.maxDepth = 1.0f;
	viewport.minDepth = 0.0f;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	vkCmdSetViewport(buffer, 0, 1, &viewport);

	VkRect2D tijeras{};
	tijeras.extent = swapChain->swapExtent;
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

	endSingleUseCmdBuffer(cmdBuffer, graphicsQueueHandle);
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
	
	if(vkCreateDescriptorSetLayout(logicalDevice, &descriptorSetLayoutCI, nullptr, &graphicsPipeline->descriptorSetLayout) != VK_SUCCESS){
		throw std::runtime_error("could not create descriptor set layout");
	}
}

void frameBufferResizeCallBack(GLFWwindow* ventanaCurr, int width, int heigth){
	windowResized = true;
}

void drawFrame(){
	//debemos sincronizar estas operaciones manualmente
	vkWaitForFences(logicalDevice, 1, &frameDrawnFences[currFrameDrawn], VK_FALSE, UINT64_MAX);//estoy dibujando 1 frame al mismo tiempo

	//conseguir framebuffer: semaforo
	uint32_t imageIndex;
	VkResult resultRelatedToSwapChain = vkAcquireNextImageKHR(logicalDevice, swapChain->handle, UINT64_MAX, gotframeBufferImageSemaforos[currFrameDrawn],VK_NULL_HANDLE, &imageIndex);
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
	matrixTransformations.proj = glm::perspective(glm::radians(90.0f), swapChain->swapExtent.width/(float)swapChain->swapExtent.height, 0.1f, 10.0f);
	matrixTransformations.proj[1][1] *= -1;//cambiamos el signo de la Y porque en vulkan la Y crece hacia abajo
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
	presentInfo.pSwapchains = &swapChain->handle;
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
	currFrameDrawn = (currFrameDrawn + 1) % MAX_FRAMES_IN_FLIGHT;//manera inteligente de loopear al inicio
}

int main() {
	printf("AAAAA"),
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

	//ya creamos la instance
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
		throw std::runtime_error("old as fuck computer");
	}

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
	
	/*command buffers*/
	//deben estar en command pools, que manejan su memoria 
	VkCommandPoolCreateInfo commandPoolCI{};
	commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCI.queueFamilyIndex = selectedQueueFamilies.graphicsFamilyIndex;
	commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//le digo que me deje actualizarlos por separado

	if (vkCreateCommandPool(logicalDevice, &commandPoolCI, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("could not create command pool");
	}

	//de paso creamos la de transfer 
	VkCommandPoolCreateInfo transferCommandPoolCI{};
	transferCommandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferCommandPoolCI.queueFamilyIndex = selectedQueueFamilies.transferFamilyIndex;
	transferCommandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//le digo que me deje actualizarlos por separado

	if (vkCreateCommandPool(logicalDevice, &transferCommandPoolCI, nullptr, &transferCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("could not create transfer command pool");
	}

	//alojamos la memoria para los buffers//automáticamente desalojados al destruir su pool
	VkCommandBufferAllocateInfo commandBufferAlocateInfo{};
	commandBufferAlocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAlocateInfo.commandPool = commandPool;
	commandBufferAlocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;//se alojan varios a la vez usualmente
	commandBufferAlocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;//esta primario (se da directamente a la queue) y secundario (lo llama un command buffer primario), esto para reusar operaciones comunes

	if (vkAllocateCommandBuffers(logicalDevice, &commandBufferAlocateInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("could not create command buffer");
	}

	/*creamos la swap chain*///esto también obtiene las handles a sus imágenes
	swapChain = std::make_unique<asgSwapChain>();	
	graphicsPipeline = std::make_unique<asgPipeline>(swapChain->surfaceFormat.format,swapChain->depthBufferFormat);
	createFrameBuffers();

	//creamos el vertexBuffer con una funcion, debe estar debajo de la creación de command pool
	createVertexBuffer();
	createIndexBuffer();

	/*creamos primitivos de sincronización*/
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
		
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
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
		createBuffer(sizeof(MatrixTransformations), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					&uniformBuffers[i], &uniformBufferMemories[i]);
		
		vkMapMemory(logicalDevice, uniformBufferMemories[i], 0, sizeof(MatrixTransformations), 0, &mappedUniformBufferMemories[i]);
	}

	/*descriptor sets*///para las uniforms necesito algo que apunte a su localización (descriptor), algo que diga como leer esa información (descriptor layout), y todos los descriptor se ponen en un set de descriptores, los cuales se crean desde pools como los commandBuffers
	VkDescriptorPoolSize descriptorPoolSize{};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);//max descriptor (not sets)

	VkDescriptorPoolSize samplerPoolSize{};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);//como está en set con las uniform, voy a necesitar repetir

	VkDescriptorPoolSize allDescriptorPoolSizes[] = {descriptorPoolSize, samplerPoolSize};
	//No haces la pool lo suficientemente grande es uno de los problemas que las validation layers de vulkan 1.1 no atrapan, regresan POOL OUT OF MEMORY
	VkDescriptorPoolCreateInfo descriptorPoolCI{};
	descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCI.pPoolSizes = allDescriptorPoolSizes;
	descriptorPoolCI.poolSizeCount = 2;
	descriptorPoolCI.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if(vkCreateDescriptorPool(logicalDevice, &descriptorPoolCI, nullptr, &descriptorPool) != VK_SUCCESS){
		throw std::runtime_error("could not crate descriptor pool");
	}
	
	//ahora que tengo la pool, puedo crearlo
	VkDescriptorSetAllocateInfo descriptorSetAI{};
	descriptorSetAI.descriptorPool = descriptorPool;
	descriptorSetAI.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	descriptorSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, graphicsPipeline->descriptorSetLayout);//vector de longitud 2 con todos los valores inicializados con descriptorSetLayout
	descriptorSetAI.pSetLayouts = descriptorSetLayouts.data();
	
	if (vkAllocateDescriptorSets(logicalDevice, &descriptorSetAI, descriptorSets.data()) != VK_SUCCESS){
		throw std::runtime_error("could not allocate descriptor sets");
	}

	/*imágenes*///como las voy a meter al mismo descriptor set las pongo aquí
	//la creamos con una función
	createTextureImage("./resourceFiles/testImages/testImage.jpg", &testImage, &testImageMemory);
	
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
	for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
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
	
	/*main loop*/
	while (!glfwWindowShouldClose(ventana)) {
		glfwPollEvents();
		drawFrame();
	}
	vkDeviceWaitIdle(logicalDevice);

	/*cleanup*/
	vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
		vkDestroyBuffer(logicalDevice, uniformBuffers[i], nullptr);
		vkFreeMemory(logicalDevice, uniformBufferMemories[i], nullptr);
	}

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
		vkDestroyFence(logicalDevice, frameDrawnFences[i], nullptr);
		vkDestroySemaphore(logicalDevice, imageWrittenSemaforos[i], nullptr);
		vkDestroySemaphore(logicalDevice, gotframeBufferImageSemaforos[i], nullptr);
	}
	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
	vkDestroyCommandPool(logicalDevice, transferCommandPool, nullptr);
	
	destroyFrameBuffers();
	swapChain->del();//frameBuffers se deben destruir antes de render pass e image views

	vkDestroyImage(logicalDevice, testImage, nullptr);
	vkFreeMemory(logicalDevice, testImageMemory, nullptr);
	vkDestroyImageView(logicalDevice, testImageView, nullptr);
	vkDestroySampler(logicalDevice, testSampler, nullptr);

	vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);
	vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
	vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);
	
	graphicsPipeline->del();
	
	vkDestroySurfaceKHR(vulkanInstance, windowSurface, nullptr);//necesita destruirse antes de su instance
	vkDestroyDevice(logicalDevice, nullptr); //ESTO DEBE DE ESTAR ANTES DE DESTROY INSTANCE
	vkDestroyInstance(vulkanInstance, nullptr);//we destroy the instance for cleanup and avoiding leaks

	glfwDestroyWindow(ventana);
	glfwTerminate();
	
	return 0;
}
