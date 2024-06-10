//La usaré metiendo a mi dependencies/lib ASGraphics.lib y a dependencies/include ASGraphics.hpp
#include <ASGraphics.hpp>

//aquí van los include que no necesita ver quien usa esto para no contaminar el enviroment
//builtin
#define NOMINMAX//para que windows no los defina
#include <cstdio>
#include <vector>
#include <cstring>//para memcpy
#include <set>
#include <limits>//para numeric limits
#include <Array>//para std::array

//dependencies
#include <STB/stb_image.h>

//local
#include <dependencies/binFileLoader.hpp>
#include <ASG_utils.hpp>
#include <ASG_swapChain.hpp>
#include <ASG_graphicsPipeline.hpp>


/*definitions echas por mi*/
#define MAX_FRAMES_IN_FLIGHT (uint32_t)2//aparently frames in flight = frames ready and waiting to be on screen
#define SCREENLONG 500
#define SCREENTALL 500

/*Structs*/

//////////*globales*////////////////

/*const*/

const std::vector<const char*> usedExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };//es un typedef string
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
const bool validationLayersEnabled = false;
#else

const bool validationLayersEnabled = true;
#endif

/*
//class declarations
class asgVIBuffer;
*/
/*var*/
/*
VkInstance vulkanInstance;

//swapchain
asgSwapChain swapChain;

//graphicsPipeline
asgPipeline graphicsPipeline;

//buffers
std::vector<VkFramebuffer>frameBuffers;
std::vector<asgVIBuffer>vertexIndexBuffers(0);
std::vector<VkBuffer>uniformBuffers(MAX_FRAMES_IN_FLIGHT);//estoy separando los buffers principalmente por tipo de memoria
std::vector<VkDeviceMemory>deviceMemories(MAX_FRAMES_IN_FLIGHT);//debería hacer 1 memoria para todas las uniform?

//uniforms
VkDescriptorPool descriptorPool;
std::vector<void*> mappedUniformBufferMemories(MAX_FRAMES_IN_FLIGHT);
std::vector<VkDescriptorSet> descriptorSets(MAX_FRAMES_IN_FLIGHT);

//cmdBuffers
std::vector<VkCommandBuffer> commandBuffers(MAX_FRAMES_IN_FLIGHT);

//ventana
bool windowResized = false;
*/
/*draw*/
//sync
/*
std::vector<VkSemaphore> gotframeBufferImageSemaforos(MAX_FRAMES_IN_FLIGHT);
std::vector<VkSemaphore>imageWrittenSemaforos(MAX_FRAMES_IN_FLIGHT);
std::vector<VkFence> frameDrawnFences(MAX_FRAMES_IN_FLIGHT);
uint32_t currFrameDrawn = 0;

//images


//class definitions
*/
/*structs*/
/*
class asgVIBuffer {
public:
	VkBuffer vertexHandle;
	VkBuffer indexHandle;

	asgVIBuffer() {
	}

	asgVIBuffer(VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize) {
		//creo vertexBuffer
		VkBufferCreateInfo VertexBufferCI;

		queueFamilyIndices queueIndices = getSelectedQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { queueIndices.transferFamilyIndex, queueIndices.graphicsFamilyIndex };
		VertexBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		VertexBufferCI.pQueueFamilyIndices = queueFamilyIndices;
		VertexBufferCI.queueFamilyIndexCount = 2;
		VertexBufferCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
		VertexBufferCI.size = vertexBufferSize;
		VertexBufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;


		if (vkCreateBuffer(logicalDevice, &VertexBufferCI, nullptr, &this->vertexHandle) != VK_SUCCESS) {
			throw std::runtime_error("could not create new vertex buffer");
		}

		// consigo sus mem requirements
		VkMemoryRequirements vertexMemRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, this->vertexHandle, &vertexMemRequirements);

		// creo indexBuffer 
		VkBufferCreateInfo indexBufferCI;
		indexBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexBufferCI.pQueueFamilyIndices = queueFamilyIndices;
		indexBufferCI.queueFamilyIndexCount = 2;
		indexBufferCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
		indexBufferCI.size = indexBufferSize;
		indexBufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		if (vkCreateBuffer(logicalDevice, &indexBufferCI, nullptr, &this->indexHandle) != VK_SUCCESS) {
			throw std::runtime_error("could not create new index buffer");
		}

		// consigo sus mem requirements
		VkMemoryRequirements indexMemRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, this->indexHandle, &indexMemRequirements);

		// alojo memoria 
		VkMemoryAllocateInfo deviceMemoryAI;

		deviceMemoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		deviceMemoryAI.memoryTypeIndex = findRigthMemoryType(vertexMemRequirements.memoryTypeBits & indexMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);//busco una memoria que sea buena para vertexBuffer y(&) 
		deviceMemoryAI.allocationSize = vertexMemRequirements.size + indexMemRequirements.size;

		if (vkAllocateMemory(logicalDevice, &deviceMemoryAI, nullptr, &this->memory) != VK_SUCCESS) {
			throw std::runtime_error("could not allocate vertex buffer memory");
		}

		vkBindBufferMemory(logicalDevice, this->vertexHandle, this->memory, 0);
		vkBindBufferMemory(logicalDevice, this->indexHandle, this->memory, static_cast<unsigned int>(indexMemRequirements.alignment * ceil(vertexMemRequirements.size / indexMemRequirements.alignment)));

		//inicializo atributos
		this->vertexByteOffset = 0;
		this->vertexByteSize = vertexBufferSize;
		this->vertexBytesUsed = 0;

		this->indexByteOffset = static_cast<unsigned int>(indexMemRequirements.alignment * ceil(vertexMemRequirements.size / indexMemRequirements.alignment));
		this->indexByteSize = indexBufferSize;
		this->indexBytesUsed = 0;
	}

	void append(std::vector<Vertex> vertexData, std::vector<uint32_t> indexData) {
		//checar si cabe vertex
		//checar si cabe index
			 //si no
				//resize
		//meter vertex e index
			//creo staging buffer
			//meto new vertex data
			//copio staging -> vertex en su offset
			//meto new index data
			// copio staging -> index en su offset

		uint32_t appendedVertexDataSize = vertexData.size() * sizeof(vertexData[0]);
		uint32_t appendedIndexDataSize = indexData.size() * sizeof(indexData[0]);

		bool vertexFits = appendedVertexDataSize < (this->vertexByteSize - this->vertexBytesUsed);
		bool indexFits = appendedIndexDataSize < (this->indexByteSize - this->indexBytesUsed);

		uint32_t allVertexDataSize = this->vertexBytesUsed + appendedVertexDataSize;
		uint32_t allIndexDataSize = this->indexBytesUsed + appendedIndexDataSize;

		if (!(vertexFits && indexFits)) {
			this->resize(allVertexDataSize, allIndexDataSize);
		}

		//staging
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;//we assume that a staging buffer of the size of the vertexData is enough to hold the index data too
		void* mappedStagingData;

		createGenericBuffer(appendedVertexDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);
		vkMapMemory(logicalDevice, stagingBufferMemory, 0, allVertexDataSize, 0, &mappedStagingData);//también podría poner VK_WHOLE_SIZE para mapear toda la memoria

		memcpy(mappedStagingData, vertexData.data(), appendedVertexDataSize);//mover los datos al gpu ocurre en el fondo, vulkan solo dice que está garantizado que ya haya ocurrido para la siguiente llamada a vkQueueSubmit, lo cual es good enough para este uso
		copyBuffer(stagingBuffer, 0, this->vertexHandle, this->vertexBytesUsed, appendedVertexDataSize);//copy buffer already waits idle

		memcpy(mappedStagingData, indexData.data(), appendedIndexDataSize);
		copyBuffer(stagingBuffer, 0, this->indexHandle, this->indexBytesUsed, appendedIndexDataSize);

		//cleanup
		vkUnmapMemory(logicalDevice, stagingBufferMemory);
		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

		this->vertexBytesUsed += appendedVertexDataSize;
		this->indexBytesUsed += appendedIndexDataSize;
	}

	void del() {
		vkDestroyBuffer(logicalDevice, this->vertexHandle, nullptr);
		vkDestroyBuffer(logicalDevice, this->indexHandle, nullptr);
		vkFreeMemory(logicalDevice, this->memory, nullptr);
	}

	~asgVIBuffer() {
		this->del();
	}

private:
	VkDeviceSize vertexByteOffset;
	VkDeviceSize vertexByteSize;
	uint32_t vertexBytesUsed;

	VkDeviceSize indexByteOffset;
	VkDeviceSize indexByteSize;
	uint32_t indexBytesUsed;

	VkDeviceMemory memory;

	void resize(VkDeviceSize vertexSizeRequired, VkDeviceSize indexSizeRequired) {//makes the buffer big enough for the new data, de momento voy a alojar solo lo q necesito
		//creo nueva memoria
			// creo vertexBuffer de tamaño size required
			// consigo sus mem requirements
			// creo indexBuffer de tamaño size required
			// consigo sus mem requirements
			// alojo memoria 
				// de tamaño requirementes1.size + requirements2.size
				// de memoryTypeIndex = findRightMemoryType(requerimientos de ambos, propiedades queridas de ambos{device local}
		//meto vertex dejando espacio para new
			// lo copio en offset 0 
		//meto index en offset
			// lo copio en ceil(requirements1.size/requirements2.alignment)

		// creo vertexBuffer de tamaño size required
		VkBuffer newVertexBuffer;
		VkBufferCreateInfo newVertexBufferCI;

		queueFamilyIndices queueIndices = getSelectedQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { queueIndices.transferFamilyIndex, queueIndices.graphicsFamilyIndex };
		newVertexBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		newVertexBufferCI.pQueueFamilyIndices = queueFamilyIndices;
		newVertexBufferCI.queueFamilyIndexCount = 2;
		newVertexBufferCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
		newVertexBufferCI.size = vertexSizeRequired;
		newVertexBufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;


		if (vkCreateBuffer(logicalDevice, &newVertexBufferCI, nullptr, &newVertexBuffer) != VK_SUCCESS) {
			throw std::runtime_error("could not create new vertex buffer");
		}

		// consigo sus mem requirements
		VkMemoryRequirements vertexMemRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, newVertexBuffer, &vertexMemRequirements);

		// creo indexBuffer de tamaño size required
		VkBuffer newIndexBuffer;
		VkBufferCreateInfo newIndexBufferCI;
		newIndexBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		newIndexBufferCI.pQueueFamilyIndices = queueFamilyIndices;
		newIndexBufferCI.queueFamilyIndexCount = 2;
		newIndexBufferCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
		newIndexBufferCI.size = indexSizeRequired;
		newIndexBufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		if (vkCreateBuffer(logicalDevice, &newIndexBufferCI, nullptr, &newIndexBuffer) != VK_SUCCESS) {
			throw std::runtime_error("could not create new index buffer");
		}

		// consigo sus mem requirements
		VkMemoryRequirements indexMemRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, newIndexBuffer, &indexMemRequirements);

		// alojo memoria 
		VkDeviceMemory newDeviceMemory;
		VkMemoryAllocateInfo newDeviceMemoryAI;

		newDeviceMemoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		newDeviceMemoryAI.memoryTypeIndex = findRigthMemoryType(vertexMemRequirements.memoryTypeBits & indexMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);//busco una memoria que sea buena para vertexBuffer y(&) 
		newDeviceMemoryAI.allocationSize = vertexMemRequirements.size + indexMemRequirements.size;

		if (vkAllocateMemory(logicalDevice, &newDeviceMemoryAI, nullptr, &newDeviceMemory) != VK_SUCCESS) {
			throw std::runtime_error("could not allocate vertex buffer memory");
		}

		vkBindBufferMemory(logicalDevice, newVertexBuffer, newDeviceMemory, 0);
		vkBindBufferMemory(logicalDevice, newIndexBuffer, newDeviceMemory, static_cast<unsigned int>(indexMemRequirements.alignment * ceil(vertexMemRequirements.size / indexMemRequirements.alignment)));

		//paso los contenidos
		copyBuffer(this->vertexHandle, 0, newVertexBuffer, 0, VK_WHOLE_SIZE);
		copyBuffer(this->indexHandle, 0, newIndexBuffer, 0, VK_WHOLE_SIZE);

		//destruyo lo anterior
		vkDestroyBuffer(logicalDevice, this->vertexHandle, nullptr);
		vkDestroyBuffer(logicalDevice, this->indexHandle, nullptr);
		vkFreeMemory(logicalDevice, this->memory, nullptr);

		//cambio las handles
		this->vertexHandle = newVertexBuffer;
		this->indexHandle = newIndexBuffer;
		this->memory = newDeviceMemory;

		//cambio los otros atributos
		this->vertexByteOffset = 0;//ps siempre es
		this->vertexByteSize = vertexSizeRequired;

		this->indexByteOffset = static_cast<unsigned int>(indexMemRequirements.alignment * ceil(vertexMemRequirements.size / indexMemRequirements.alignment));//para q vrgs necesito estos offsets?
		this->indexByteSize = indexSizeRequired;
	}
};

*/
/////*functions*////////

/*declarations*/
/*
//helpers
bool isPhysicalDeviceSuitable(VkPhysicalDevice device);
bool checkValidationLayerSupport();
bool checkDeviceExtensionSupport(VkPhysicalDevice device);
void createFrameBuffers();
void destroyFrameBuffers();

//init
void frameBufferResizeCallBack(GLFWwindow* ventanaCurr, int width, int heigth);

//general
*/
/*definitions*/
/*
//helpers
bool isPhysicalDeviceSuitable(VkPhysicalDevice device) {
	queueFamilyIndices selectedQueueFamilies = getSelectedQueueFamilies(device);
	//checamos extensiones, features y la swapChain
	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool featuresSupported = false;
	VkPhysicalDeviceFeatures physicalDeviceSupportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &physicalDeviceSupportedFeatures);

	if (physicalDeviceSupportedFeatures.samplerAnisotropy) {
		featuresSupported = true;
	}

	bool swapChainHasAllRequirements = false;
	if (extensionsSupported) {//debemos checar la swap chain solo si ya nos aseguramos que su extensión si tiene support

		SwapChainSupportDetails swapChainDetails = getSwapChainSupportDetails(device);
		swapChainHasAllRequirements = !swapChainDetails.formats.empty() && !swapChainDetails.presentModes.empty();
	}

	return (selectedQueueFamilies.hasAllRequirements() && extensionsSupported && swapChainHasAllRequirements);//ponemos extensions supported por completness
}
bool checkValidationLayerSupport() {
	//conseguimos las layers que si puedo usar
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> supportedLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, supportedLayers.data());

	//checamos si las validation layers están en la lista de supported layers
	for (const char* layer : validationLayers) {
		bool layerIsSupported = false;
		for (const auto& currSupportedLayer : supportedLayers) {
			if (strcmp(layer, currSupportedLayer.layerName) == 0) {
				layerIsSupported = true;
			}
		}
		if (!layerIsSupported) {
			return false;
		}
	}
	return true;
}
bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t numExtension;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtension, nullptr);

	std::vector<VkExtensionProperties> supportedExtensions(numExtension);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtension, supportedExtensions.data());

	//checamos si las extensiones que necesitamos están en las extensiones que tienen soporte, podrías hacer 2 for loop, pero ps no
	std::set<std::string> requiredExtensions(usedExtensions.begin(), usedExtensions.end());

	for (const auto& extension : supportedExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}
*/

//init

//general
/*
void asgLoadData(std::vector<Vertex> vertices, std::vector<unsigned int> indices) {
	vertexIndexBuffers.push_back(asgVIBuffer(1000, 1000));
	vertexIndexBuffers[vertexIndexBuffers.size() - 1].append(vertices, indices);
}
void drawFrame() {
	for (asgVIBuffer vib : vertexIndexBuffers) {

	}

	//debemos sincronizar estas operaciones manualmente
	vkWaitForFences(logicalDevice, 1, &frameDrawnFences[currFrameDrawn], VK_FALSE, UINT64_MAX);//estoy dibujando 1 frame al mismo tiempo

	//conseguir framebuffer: semaforo
	uint32_t imageIndex;
	VkResult resultRelatedToSwapChain = vkAcquireNextImageKHR(logicalDevice, swapChain.handle, UINT64_MAX, gotframeBufferImageSemaforos[currFrameDrawn], VK_NULL_HANDLE, &imageIndex);
	if (resultRelatedToSwapChain == VK_ERROR_OUT_OF_DATE_KHR) {
		//remake swapChain
		waitUntilCanRemakeSwapChain();
		destroyFrameBuffers();
		swapChain = asgSwapChain();
		createFrameBuffers();
		return;
	}
	else if (resultRelatedToSwapChain != VK_SUCCESS && resultRelatedToSwapChain != VK_SUBOPTIMAL_KHR) {//im treating suboptimal as good enough
		throw std::runtime_error("could not get swapChain next image");
	}

	//we are here wich means we are actually working so:
	vkResetFences(logicalDevice, 1, &frameDrawnFences[currFrameDrawn]);

	//grabar comandos al frame: ocurre en cpu, no necesita sincronización
	vkResetCommandBuffer(commandBuffers[currFrameDrawn], 0);//el segundo parámetro es una bitmask para flags
	
	//begin recording
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;//opcional
	beginInfo.pInheritanceInfo = nullptr;//es para buffers secundarios

	if (vkBeginCommandBuffer(commandBuffers[currFrameDrawn], &beginInfo) != VK_SUCCESS) {//lo resetea implicitamente
		throw std::runtime_error("could not begin recording commands");
	}
	*/
	/*begin render pass*/
	/*
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = graphicsPipeline.renderPass;
	renderPassInfo.framebuffer = frameBuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent = swapChain.swapExtent;

	//clear value
	VkClearValue clearValues[2];//EL ORDEN DE ESTOS CLEAR VALUES DEBE SER IDENTICO AL DE LOS ATTACHMENTS
	clearValues[0].color = { 0.0f,0.0f,0.0f };//POSIBLE BUG SOURCE PQ NO INICALIZO LOS STRUCTS
	clearValues[1].depthStencil = { 1.0f, 0 };//los pongo todos en 1.0f, el valor de profundidad más lejano, el 0 es stencil



	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	//iniciar la render pass
	vkCmdBeginRenderPass(commandBuffers[currFrameDrawn], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);//3er argumento, los comandos de la render pass serán embedded en el buffer sin usar un buffer secundario
	vkCmdBindPipeline(commandBuffers[currFrameDrawn], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.handle);

	VkBuffer buffers[] = { vertexIndexBuffers[0].vertexHandle};
	VkDeviceSize bufferOffsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffers[currFrameDrawn], 0, 1, buffers, bufferOffsets);
	vkCmdBindIndexBuffer(commandBuffers[currFrameDrawn], vertexIndexBuffers[0].indexHandle, 0, VK_INDEX_TYPE_UINT32);
	//TODO vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currFrameDrawn], 0, nullptr);//tengo que especificar si a la graphics o compute pipeline
	
	//ViewPort y Scissors, pq los pusimos como dynamic state, realmente lo quiero en dynamic state? TODO
	VkViewport viewport{};
	viewport.height = static_cast<float>(swapChain.swapExtent.height);
	viewport.width = static_cast<float>(swapChain.swapExtent.width);
	viewport.maxDepth = 1.0f;
	viewport.minDepth = 0.0f;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	vkCmdSetViewport(commandBuffers[currFrameDrawn], 0, 1, &viewport);

	VkRect2D tijeras{};
	tijeras.extent = swapChain.swapExtent;
	tijeras.offset = { 0,0 };
	vkCmdSetScissor(commandBuffers[currFrameDrawn], 0, 1, &tijeras);

	//draw and end
	vkCmdDrawIndexed(commandBuffers[currFrameDrawn], 3, 1, 0, 0, 0);
	vkCmdEndRenderPass(commandBuffers[currFrameDrawn]);

	if (vkEndCommandBuffer(commandBuffers[currFrameDrawn]) != VK_SUCCESS) {
		throw std::runtime_error("could not end recording of cmd buffer");
	}
	*/
	/*
	grabarCommandBuffer(commandBuffers[currFrameDrawn], imageIndex);

	static int currTime = 0;
	currTime += 1;
	//actualizo las matrices//poner {} en matrixTransformations no las inicia como identidad
	MatrixTransformations matrixTransformations;//maybe all 3 matrices will change, view because of camera movement and proj because of window resize
	matrixTransformations.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	matrixTransformations.proj = glm::perspective(glm::radians(90.0f), chosenSwapExtent.width / (float)chosenSwapExtent.height, 0.1f, 10.0f);
	matrixTransformations.proj[1][1] *= -1;//cambiamos el signo de la Y porque en vulkan y crece hacia abajo
	matrixTransformations.model = glm::rotate(glm::mat4(1.0f), currTime * glm::radians(5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	memcpy(mappedUniformBufferMemories[currFrameDrawn], &matrixTransformations, sizeof(matrixTransformations));
	*/
	//submito el frame buffer: semaforo
	/*
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currFrameDrawn];

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &gotframeBufferImageSemaforos[currFrameDrawn];//Esperamos al semáforo 1 en la stage 1 de ambos array, en el semáforo 2 en la stage 2 y así
	VkPipelineStageFlags stagesToWaitIn[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };//espera en esta stage especificamente
	submitInfo.pWaitDstStageMask = stagesToWaitIn;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &imageWrittenSemaforos[currFrameDrawn];

	if (vkQueueSubmit(graphicsQueueHandle, 1, &submitInfo, frameDrawnFences[currFrameDrawn]) != VK_SUCCESS) {
		throw std::runtime_error("could not submit command buffer");
	}

	//regresar la imagen a la swapChain
	VkPresentInfoKHR presentInfo{};
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pSwapchains = &swapChain.handle;
	presentInfo.pWaitSemaphores = &imageWrittenSemaforos[currFrameDrawn];
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.swapchainCount = 1;
	presentInfo.pResults = nullptr; //opcional, es un output para checar como le fué a cada swapChain individualmente

	resultRelatedToSwapChain = vkQueuePresentKHR(presentQueueHandle, &presentInfo);
	if (resultRelatedToSwapChain == VK_ERROR_OUT_OF_DATE_KHR || resultRelatedToSwapChain == VK_SUBOPTIMAL_KHR || windowResized) {
		waitUntilCanRemakeSwapChain();
		destroyFrameBuffers();
		swapChain = asgSwapChain();
		createFrameBuffers();
		windowResized = false;
	}
	else if (resultRelatedToSwapChain != VK_SUCCESS) {
		throw std::runtime_error("error with queue present");
	}
	currFrameDrawn = (currFrameDrawn + 1) % MAX_FRAMES_IN_FLIGHT;//manera inteligente de loopear al inicio
}
	*/

/*public*/
//
//void asgInit() {
//	printf("AAAAA");
//	std::cout << "validation layers enabled: " << validationLayersEnabled << std::endl;
//	
//	/*window creation*/
//	glfwInit();
//	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);//como es vulkan tenemos que hacer algo especial para rezizable windows
//	ventana = glfwCreateWindow(SCREENLONG, SCREENTALL, "uno", NULL, NULL);
//	glfwSetFramebufferSizeCallback(ventana, frameBufferResizeCallBack);

	/*crear applicación*/
	/*
	// app info
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "App";
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
	appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
	appInfo.pEngineName = "non";

	//InstanceInfo
	VkInstanceCreateInfo instanceCI{};
	instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCI.pApplicationInfo = &appInfo;

	//glfw consigue las extensiones necesarias
	uint32_t glfwExtensionsCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

	instanceCI.enabledExtensionCount = glfwExtensionsCount;
	instanceCI.ppEnabledExtensionNames = glfwExtensions;

	if (validationLayersEnabled) {
		instanceCI.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceCI.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		instanceCI.enabledLayerCount = 0;
	}

	//cheacar si si están bien las validation layers
	if (validationLayersEnabled && !checkValidationLayerSupport()) {
		throw std::runtime_error("one or more validation layers are not supported");
	}
	else {
		printf("\nno problem with validation layers\n");
	}

	if (vkCreateInstance(&instanceCI, nullptr, &vulkanInstance) != VK_SUCCESS) {
		throw std::runtime_error("could not create vulkan instance");
	}
	*/
	/*Creamos window surface*/ //debe ser creada justo después de la instance porque influencia la decisión de physical device
	/*
	if (glfwCreateWindowSurface(vulkanInstance, ventana, nullptr, &windowSurface) != VK_SUCCESS) {//glfw nos ayuda a saltarnos 30000000 lineas
		throw std::runtime_error("could not create window surface");
	}
	else {
		printf("\nwindow surface created\n");
	}
	*/
	/*elegimos la targeta gráfica (physical device)*/
	/*
	//conseguimos lista de devices
	uint32_t ndevices = 0;
	vkEnumeratePhysicalDevices(vulkanInstance, &ndevices, nullptr);

	if (ndevices == 0) {
		throw std::runtime_error("old as fuck computer");
	}

	std::vector<VkPhysicalDevice> allDevices(ndevices);
	vkEnumeratePhysicalDevices(vulkanInstance, &ndevices, allDevices.data());

	//buscamos un device que nos sirva
	for (const auto& currDevice : allDevices) {
		if (isPhysicalDeviceSuitable(currDevice)) {
			physicalDevice = currDevice;
			break;//podriamos no poner este break si quieres obtener todos los devices que son compatibles
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("could not pick a device, you have no decent GPUs");
	}
	else {
		printf("\nappropiate device found\n");
	}
	*/
	/*hacemos logical device*/
	/*
	//especificamos las queues que se crearán
	queueFamilyIndices selectedQueueFamilies = getSelectedQueueFamilies(physicalDevice);
	float allQueuePriority = 1.0f;//todas van a tener la misma prioridad de momento

	//hacemos la createInfo de cada queue
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(0);
	std::set<uint32_t> uniqueQueueIndices(selectedQueueFamilies.allFamilyIndices, selectedQueueFamilies.allFamilyIndices + sizeof(selectedQueueFamilies.allFamilyIndices) / sizeof(uint32_t));
	for (uint32_t familyIndex : uniqueQueueIndices) {
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
	VkDeviceCreateInfo logicalDeviceCI{};
	logicalDeviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logicalDeviceCI.queueCreateInfoCount = static_cast<uint32_t>(uniqueQueueIndices.size());
	logicalDeviceCI.pQueueCreateInfos = queueCreateInfos.data();
	logicalDeviceCI.pEnabledFeatures = &deviceFeaturesUsed;

	//Las siguientes están deprecadas?, pero para compatibilidad se pueden poner
	logicalDeviceCI.enabledExtensionCount = static_cast<uint32_t>(usedExtensions.size());
	logicalDeviceCI.ppEnabledExtensionNames = usedExtensions.data();
	if (validationLayersEnabled) {
		logicalDeviceCI.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		logicalDeviceCI.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		logicalDeviceCI.enabledLayerCount = 0;
	}

	//lo creamos
	if (vkCreateDevice(physicalDevice, &logicalDeviceCI, nullptr, &logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("could not create logical device");
	}
	else {
		printf("\nlogical device created\n");
	}

	//conseguimos las queueHandles que se hicieron al mismo tiempo que el logical device
	vkGetDeviceQueue(logicalDevice, selectedQueueFamilies.graphicsFamilyIndex, 0, &graphicsQueueHandle); // en queue index va su indice de queue de esta familia, solo tenemos uno de cada familia así que es 0 en todos.
	vkGetDeviceQueue(logicalDevice, selectedQueueFamilies.presentFamilyIndex, 0, &presentQueueHandle);
	vkGetDeviceQueue(logicalDevice, selectedQueueFamilies.transferFamilyIndex, 0, &transferQueueHandle);
	
	swapChain = asgSwapChain();
	createFrameBuffers();
	graphicsPipeline = asgPipeline(swapChain.surfaceFormat.format,swapChain.depthBufferFormat);
	*/
	/*command buffers*/
//	//deben estar en command pools, que manejan su memoria 
//	VkCommandPoolCreateInfo commandPoolCI{};
//	commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//	commandPoolCI.queueFamilyIndex = selectedQueueFamilies.graphicsFamilyIndex;
//	commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//le digo que me deje actualizarlos por separado
//
//	if (vkCreateCommandPool(logicalDevice, &commandPoolCI, nullptr, &commandPool) != VK_SUCCESS) {
//		throw std::runtime_error("could not create command pool");
//	}
//
//	//de paso creamos la de transfer 
//	VkCommandPoolCreateInfo transferCommandPoolCI{};
//	transferCommandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//	transferCommandPoolCI.queueFamilyIndex = selectedQueueFamilies.transferFamilyIndex;
//	transferCommandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//le digo que me deje actualizarlos por separado
//
//	if (vkCreateCommandPool(logicalDevice, &transferCommandPoolCI, nullptr, &transferCommandPool) != VK_SUCCESS) {
//		throw std::runtime_error("could not create transfer command pool");
//	}
//
//
//	/*Alojamos los command buffers locales*/
//	//alojamos la memoria para los buffers//automáticamente desalojados al destruir su pool
//	VkCommandBufferAllocateInfo commandBufferAlocateInfo{};
//	commandBufferAlocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//	commandBufferAlocateInfo.commandPool = commandPool;
//	commandBufferAlocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;//se alojan varios a la vez usualmente
//	commandBufferAlocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;//esta primario (se da directamente a la queue) y secundario (lo llama un command buffer primario), esto para reusar operaciones comunes
//
//	if (vkAllocateCommandBuffers(logicalDevice, &commandBufferAlocateInfo, commandBuffers.data()) != VK_SUCCESS) {
//		throw std::runtime_error("could not create command buffer");
//	}
//
//	/*creamos primitivos de sincronización*/
//	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
//
//		VkFenceCreateInfo fenceCI{};//de echo estos primitivos no tienen parámetros, esto es para forward compatibility
//		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
//		fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;//ps pa que en el primer frame diga inmediatamente que ya terminó de dibujar el anterior
//
//		VkSemaphoreCreateInfo semaforoCI{};
//		semaforoCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
//
//		if (vkCreateFence(logicalDevice, &fenceCI, nullptr, &frameDrawnFences[i]) != VK_SUCCESS
//			|| vkCreateSemaphore(logicalDevice, &semaforoCI, nullptr, &gotframeBufferImageSemaforos[i]) != VK_SUCCESS
//			|| vkCreateSemaphore(logicalDevice, &semaforoCI, nullptr, &imageWrittenSemaforos[i]) != VK_SUCCESS) {
//			throw std::runtime_error("could not create sync primitives");
//		}
//	}
//}
//
//void asgTerminate() {
//	
//	destroyFrameBuffers();//deben eliminarse antes de la swapChain
//	swapChain.del();
//	graphicsPipeline.del();
//	vkDestroySurfaceKHR(vulkanInstance, windowSurface, nullptr);//necesita destruirse antes de su instance
//	vkDestroyDevice(logicalDevice, nullptr); //ESTO DEBE DE ESTAR ANTES DE DESTROY INSTANCE
//	vkDestroyInstance(vulkanInstance, nullptr);//we destroy the instance for cleanup and avoiding leaks
//
//	glfwDestroyWindow(ventana);
//	glfwTerminate();
//}
////COMENTARIO SOLO PARA VER EL NÚMERO DE LINEAS