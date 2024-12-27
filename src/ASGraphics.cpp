//La usar� metiendo a mi dependencies/lib ASGraphics.lib y a dependencies/include ASGraphics.hpp
#include <ASGraphics.hpp>

//aqu� van los include que no necesita ver quien usa esto para no contaminar el enviroment
//builtin
#define NOMINMAX//para que windows no los defina
#include <cstdio>
#include <vector>
#include <cstring>//para memcpy
#include <set>
#include <limits>//para numeric limits
#include <Array>//para std::array
#include <map>
#include <algorithm>
#include <random>//TODO

//dependencies
#include <STB/stb_image.h>//me gustaria incuir glfw explicitamente, pero debes de #define include vulkan, tonce mejor q utils abstraiga eso
#include <tiny_gltf.h>

//local
#include <dependencies/binFileLoader.hpp>
#include <ASG_utils.hpp>
#include <ASG_descriptorSets.hpp>
#include <ASG_swapChain.hpp>
#include <ASG_VertexIndexBuffer.hpp>
#include <ASG_imageHandler.hpp>
#include <ASG_model.hpp>
#include <ASG_renderPass.hpp>
#include <ASG_renderPass_deffered.hpp>


/*definitions echas por mi*/

/*Structs*/

/*class declarations*/


//////////*globales*////////////////

/*const*/
const std::vector<const char*> usedExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };//es un typedef string
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
const bool validationLayersEnabled = false;//TODO creo q esto no funciona como extern porque no tiene extern
#else
const bool validationLayersEnabled = true;
#endif

/*var*/
VkInstance vulkanInstance;

//swapchain
std::unique_ptr<asgSwapChain> swapChain;

//buffers
std::map<std::string, std::unique_ptr<asgVIBuffer>> materialsBuffers;
std::map<std::string, std::vector<asgPrimitive*>> materialsPrimitives;

//cmdBuffers
std::vector<VkCommandBuffer> commandBuffers(MAX_FRAMES_IN_FLIGHT);

//ventana
bool windowResized = false;

/*draw*/
//sync
std::vector<VkSemaphore> gotframeBufferImageSemaforos(MAX_FRAMES_IN_FLIGHT);
std::vector<VkSemaphore>imageWrittenSemaforos(MAX_FRAMES_IN_FLIGHT);
std::vector<VkFence> frameDrawnFences(MAX_FRAMES_IN_FLIGHT);
uint32_t currFrameDrawn = 0;

//images


//Modelos
std::vector<asgModel> models;

///////*functions*////////

///*declarations*/
////helpers
bool isPhysicalDeviceSuitable(VkPhysicalDevice device);
bool checkValidationLayerSupport();
bool checkDeviceExtensionSupport(VkPhysicalDevice device);
void createFrameBuffers();
void destroyFrameBuffers();
void remakeSwapChain();

//init
void frameBufferResizeCallBack(GLFWwindow* ventanaCurr, int width, int heigth);
//general

/*definitions*/
//helpers
void frameBufferResizeCallBack(GLFWwindow* ventanaCurr, int width, int heigth) {
	windowResized = true;
}

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
	if (extensionsSupported) {//debemos checar la swap chain solo si ya nos aseguramos que su extensi�n si tiene support

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

	//checamos si las validation layers est�n en la lista de supported layers
	for (const char* layer : validationLayers) {
		bool layerIsSupported = false;
		for (const auto& currSupportedLayer : supportedLayers) {
			if (strcmp(layer, currSupportedLayer.layerName) == 0) {//TODO optimizar?
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

	//checamos si las extensiones que necesitamos est�n en las extensiones que tienen soporte, podr�as hacer 2 for loop, pero ps no
	std::set<std::string> requiredExtensions(usedExtensions.begin(), usedExtensions.end());

	for (const auto& extension : supportedExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

void remakeSwapChain() {
	waitUntilCanRemakeSwapChain();
	destroyFrameBuffers();
	swapChain->del();
	swapChain = std::make_unique<asgSwapChain>();
	createFrameBuffers();
}

//init
void createFrameBuffers() {
	for (auto& renderPass : renderPasses) {
		renderPass.createFrameBuffers(*swapChain.get());
	}
}
void destroyFrameBuffers() {	
	for (auto& renderPass : renderPasses) {//debemos destruirlo antes de la render pass e image views
		renderPass.destroyFrameBuffers();
	}
}

//general
asgModelHandle::asgModelHandle(uint32_t modelIndex)
{
	this->modelIndex = modelIndex;
	this->modelMatrix = glm::mat4(1.0f);
}

const uint32_t asgModelHandle::getIndex()
{
	return this->modelIndex;
}

/*public*/
void asgInit() {
	if (validationLayersEnabled) {
		printf("AAAAA");
	}

	/*window creation*/
	glfwInit();
	
	// Get the primary monitor
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	if (!monitor) {
		glfwTerminate();
		throw std::runtime_error("Failed to get monitor\n");
	}

	// Get the video mode of the monitor
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	if (!mode) {
		glfwTerminate();
		throw std::runtime_error("Failed to get video mode\n");
	}

	// Create a fullscreen window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);//como es vulkan tenemos que hacer algo especial para rezizable windows
	//ventana = glfwCreateWindow(mode->width, mode->height, "Fullscreen Window", monitor, nullptr);

	//create non fullscreen window for debugging
	ventana = glfwCreateWindow(500, 500, "window", nullptr, nullptr);

	glfwSetFramebufferSizeCallback(ventana, frameBufferResizeCallBack);
	/*crear applicaci�n*/

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

	//cheacar si si est�n bien las validation layers
	if (validationLayersEnabled && !checkValidationLayerSupport()) {
		throw std::runtime_error("one or more validation layers are not supported");
	}

	if (vkCreateInstance(&instanceCI, nullptr, &vulkanInstance) != VK_SUCCESS) {
		throw std::runtime_error("could not create vulkan instance");
	}

	/*Creamos window surface*/ //debe ser creada justo despu�s de la instance porque influencia la decisi�n de physical device
	if (glfwCreateWindowSurface(vulkanInstance, ventana, nullptr, &windowSurface) != VK_SUCCESS) {//glfw nos ayuda a saltarnos 30000000 lineas
		throw std::runtime_error("could not create window surface");
	}

	/*elegimos la targeta gr�fica (physical device)*/
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

	/*hacemos logical device*/
	//especificamos las queues que se crear�n
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

	//especificamos las features que se usar�n
	VkPhysicalDeviceFeatures deviceFeaturesUsed{};//ninguna en especial as� que dejamos todo en VK_FALSE
	deviceFeaturesUsed.samplerAnisotropy = VK_TRUE;//Character development

	///hacemos createinfo
	VkDeviceCreateInfo logicalDeviceCI{};
	logicalDeviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logicalDeviceCI.queueCreateInfoCount = static_cast<uint32_t>(uniqueQueueIndices.size());
	logicalDeviceCI.pQueueCreateInfos = queueCreateInfos.data();
	logicalDeviceCI.pEnabledFeatures = &deviceFeaturesUsed;

	//Las siguientes est�n deprecadas?, pero para compatibilidad se pueden poner
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

	//conseguimos las queueHandles que se hicieron al mismo tiempo que el logical device
	vkGetDeviceQueue(logicalDevice, selectedQueueFamilies.graphicsFamilyIndex, 0, &graphicsQueueHandle); // en queue index va su indice de queue de esta familia, solo tenemos uno de cada familia as� que es 0 en todos.
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

	/*Alojamos los command buffers locales*/
	//alojamos la memoria para los buffers//autom�ticamente desalojados al destruir su pool
	VkCommandBufferAllocateInfo commandBufferAlocateInfo{};
	commandBufferAlocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAlocateInfo.commandPool = commandPool;
	commandBufferAlocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;//se alojan varios a la vez usualmente
	commandBufferAlocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;//esta primario (se da directamente a la queue) y secundario (lo llama un command buffer primario), esto para reusar operaciones comunes

	if (vkAllocateCommandBuffers(logicalDevice, &commandBufferAlocateInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("could not create command buffer");
	}

	swapChain = std::make_unique<asgSwapChain>();
	createRenderPasses(*swapChain.get());

	/*creamos primitivos de sincronización*/
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		VkFenceCreateInfo fenceCI{};//de echo estos primitivos no tienen par�metros, esto es para forward compatibility
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;//ps pa que en el primer frame diga inmediatamente que ya termin� de dibujar el anterior

		VkSemaphoreCreateInfo semaforoCI{};
		semaforoCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateFence(logicalDevice, &fenceCI, nullptr, &frameDrawnFences[i]) != VK_SUCCESS
			|| vkCreateSemaphore(logicalDevice, &semaforoCI, nullptr, &gotframeBufferImageSemaforos[i]) != VK_SUCCESS
			|| vkCreateSemaphore(logicalDevice, &semaforoCI, nullptr, &imageWrittenSemaforos[i]) != VK_SUCCESS) {
			throw std::runtime_error("could not create sync primitives");
		}
	}

	//initializeDescriptorSets(&renderPasses[0].subPasses[0].pipeline);//TODO
	asgImageHandler::initializeResources();

	//inicializo las matrices
	MatrixTransformations matrixTransformations;//maybe all 3 matrices will change, view because of camera movement and proj because of window resize
	matrixTransformations.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	matrixTransformations.proj = glm::perspective(glm::radians(90.0f), swapChain->swapExtent.width / (float)swapChain->swapExtent.height, 0.01f, 500.0f);
	matrixTransformations.proj[1][1] *= -1;//cambiamos el signo de la Y porque en vulkan la Y crece hacia abajo

	for (int i = 0; i < defferedPassFunc::gBufferSubPass::mappedUniformBufferMemories.size(); i++) {
		memcpy(defferedPassFunc::gBufferSubPass::mappedUniformBufferMemories[i], &matrixTransformations, sizeof(matrixTransformations));
	}


	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
}

bool asgWindowShouldClose()
{
	return glfwWindowShouldClose(ventana);
}

GLFWwindow* asgGetGlfwWindowHandle()
{
	return ventana;
}

void asgPollGLFWEvents()
{
	glfwPollEvents();
}

asgModelHandle asgLoadModel(std::string pathToModel) {
	//get the data with tinygltf
	tinygltf::TinyGLTF loader;
	std::string err, warning;

	tinygltf::Model tgModel;
	if (pathToModel.back() == 'f') {
		if (validationLayersEnabled) {
			printf("\n[loading gltf]");
		}
		if (!loader.LoadASCIIFromFile(&tgModel, &err, &warning, pathToModel)) {
			throw std::runtime_error("could not load gltf file");
		}
	}
	else {
		if (validationLayersEnabled) {
			printf("\n[loading glb]");
		}
		if (!loader.LoadBinaryFromFile(&tgModel, &err, &warning, pathToModel)) {
			throw std::runtime_error("could not load gltf file");
		}
	}
	if (validationLayersEnabled) {
		std::cout << err;
		printf("\namount of scenes:%zu, default scene:%i\n", tgModel.scenes.size(), tgModel.defaultScene);
	}

	//make model
	models.emplace_back(pathToModel, tgModel);
	
	//make the model handle
	asgModelHandle returnHandle(static_cast<uint32_t>(models.size() - 1));

	if (validationLayersEnabled) {
		printf("\nIF A MODELS MATERIAL LOOK GLITCHED ITS BECAUSE THE SWITCH FOR ALBEDO MAPS DOESNT SUPPORT ITS INDEX");
	}
	return returnHandle;
}

void asgDrawFrame(glm::mat4 viewMatrix, std::vector<asgModelHandle> modelsToDraw) {
	//wait until can draw curr frame
	vkWaitForFences(logicalDevice, 1, &frameDrawnFences[currFrameDrawn], VK_FALSE, UINT64_MAX);//estoy dibujando 1 frame al mismo tiempo

	//conseguir framebuffer: se�aliza got frame buffer image semaforo
	uint32_t imageIndex;
	VkResult resultRelatedToSwapChain = vkAcquireNextImageKHR(logicalDevice, swapChain->handle, UINT64_MAX, gotframeBufferImageSemaforos[currFrameDrawn], VK_NULL_HANDLE, &imageIndex);
	if (resultRelatedToSwapChain == VK_ERROR_OUT_OF_DATE_KHR) {
		remakeSwapChain();
		return;
	}
	else if (resultRelatedToSwapChain != VK_SUCCESS && resultRelatedToSwapChain != VK_SUBOPTIMAL_KHR) {//im treating suboptimal as good enough
		throw std::runtime_error("could not get swapChain next image");
	}

	//get this frames cmdBuffer
	//VkCommandBuffer currCmdBuffer = commandBuffers[currFrameDrawn];

	//reset frame specific resources
	vkResetFences(logicalDevice, 1, &frameDrawnFences[currFrameDrawn]);//we are here wich means we are actually working so:
	vkResetCommandBuffer(commandBuffers[currFrameDrawn], 0);//el segundo par�metro es una bitmask para flags

	for (auto& modelHandle : modelsToDraw) {
		//actualiza las matrices de las meshes que se dibujaran
		models[modelHandle.getIndex()].applyMat(modelHandle.modelMatrix);

		//marcar los primitives que se van a dibujar
		for (auto& mesh : models[modelHandle.getIndex()].meshes) {
			for (auto& primitive : mesh.primitives) {
				primitive.toBeDrawn = true;//TODO IDEA: each mesh has the name of its material, i create the materialsMeshes map each drawFrameCall and fill it only with the ones to be drawn, this would help not having the mesh have a reference to its dad, tendre que hacer profiling para saber cual es mejor
			}
		}
	}

	//paso las matrices al uniform buffer
	int loadedMatrices = 0;
	for (auto& materialPrimitivePair : materialsPrimitives) {//printf("material: %s", materialPrimitivePair.first.c_str());
		for (auto& primitive : materialPrimitivePair.second) {
			if (primitive->toBeDrawn) {//only load it if the primitive is being drawn
				memcpy(static_cast<void*>(static_cast<unsigned char*>(defferedPassFunc::gBufferSubPass::mappedMeshModelMatricesBuffers[currFrameDrawn]) + loadedMatrices * 64), &primitive->currTransform, 64);//64 is the size of glm::mat4
				primitive->matrixIndex = loadedMatrices;
				loadedMatrices++;
			}

		}

	}

	//inicializo las matrices
	MatrixTransformations matrixTransformations;//maybe all 3 matrices will change, view because of camera movement and proj because of window resize
	matrixTransformations.view = viewMatrix;
	matrixTransformations.proj = glm::perspective(glm::radians(90.0f), swapChain->swapExtent.width / (float)swapChain->swapExtent.height, 0.01f, 500.0f);
	matrixTransformations.proj[1][1] *= -1;//cambiamos el signo de la Y porque en vulkan la Y crece hacia abajo

	for (int i = 0; i < defferedPassFunc::gBufferSubPass::mappedUniformBufferMemories.size(); i++) {
		memcpy(defferedPassFunc::gBufferSubPass::mappedUniformBufferMemories[i], &matrixTransformations, sizeof(matrixTransformations));
	}

	//iniciar grabacion de cmd buffer
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;//opcional
	beginInfo.pInheritanceInfo = nullptr;//es para buffers secundarios

	if (vkBeginCommandBuffer(commandBuffers[currFrameDrawn], &beginInfo) != VK_SUCCESS) {//lo resetea implicitamente
		throw std::runtime_error("could not begin recording commands");
	}

	//iniciar render pass
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPasses[0].handle;
	renderPassInfo.framebuffer = renderPasses[0].framebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent = swapChain->swapExtent;

	std::vector<VkClearValue> clearValues(4);//EL ORDEN DE ESTOS CLEAR VALUES DEBE SER IDENTICO AL DE LOS ATTACHMENTS
	clearValues[0].color = { 0.4f,0.2f,0.1f };//POSIBLE BUG SOURCE PQ NO INICALIZO LOS STRUCTS out		(swapchain)
	clearValues[1].color = { 0.0f,0.0f,0.0f };//POSIBLE BUG SOURCE PQ NO INICALIZO LOS STRUCTS normal	(frameBuffer att)
	clearValues[2].depthStencil = { 1.0f, 0 };//los pongo todos en 1.0f, el valor de profundidad m�s lejano, el 0 es stencil
	clearValues[3].color = { 0.7f,0.3f,0.7f };//POSIBLE BUG SOURCE PQ NO INICALIZO LOS STRUCTS albedo	(framebuffer att)

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffers[currFrameDrawn], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);//3er argumento: los comandos de la render pass ser�n embedded en el buffer sin usar un buffer secundario

	//bind pipeline
	vkCmdBindPipeline(commandBuffers[currFrameDrawn], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPasses[0].subPasses[0].pipeline.handle);

	//bind descriptor sets
	vkCmdBindDescriptorSets(commandBuffers[currFrameDrawn], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPasses[0].subPasses[0].pipeline.pipelineLayout, 0, 1, &renderPasses[0].subPasses[0].descriptorSets[currFrameDrawn], 0, nullptr);//tengo que especificar si a la graphics o compute pipeline

	//set dynamic state
	VkViewport viewport{};
	viewport.height = static_cast<float>(swapChain->swapExtent.height);
	viewport.width = static_cast<float>(swapChain->swapExtent.width);
	viewport.maxDepth = 1.0f;
	viewport.minDepth = 0.0f;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	vkCmdSetViewport(commandBuffers[currFrameDrawn], 0, 1, &viewport);

	VkRect2D tijeras{};
	tijeras.extent = swapChain->swapExtent;
	tijeras.offset = { 0,0 };
	vkCmdSetScissor(commandBuffers[currFrameDrawn], 0, 1, &tijeras);

	//meto las push constants generales
	float randomCoordinate = 0.0f;//((float)rand() / RAND_MAX);//static float randomCoordinate = 0.0f;//randomCoordinate += 0.01f;if (randomCoordinate >= 1.0f) { randomCoordinate = 0.0f; };
	vkCmdPushConstants(commandBuffers[currFrameDrawn], renderPasses[0].subPasses[0].pipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(pushConstants), 4, &randomCoordinate);//TODO
	
	//Dibujar: gBuffer subPass
	for (const auto& currMaterialBufferPair : materialsBuffers) {
		//conectar buffers
		VkBuffer vertexBuffers[] = { currMaterialBufferPair.second->vertexHandle };
		VkDeviceSize bufferOffsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[currFrameDrawn], 0, 1, vertexBuffers, bufferOffsets);
		vkCmdBindIndexBuffer(commandBuffers[currFrameDrawn], currMaterialBufferPair.second->indexHandle, 0, VK_INDEX_TYPE_UINT32);

		for (const auto& currPrimitive : materialsPrimitives.at(currMaterialBufferPair.first)) {
			//si esta marcada para dibujar
			if (currPrimitive->toBeDrawn) {
	
				//meto sus push constants
				pushConstants pc{};
				pc.albedoIndex = currPrimitive->pbrIndices.albedoIndex;
				pc.matrixIndex = currPrimitive->matrixIndex;
				vkCmdPushConstants(commandBuffers[currFrameDrawn], renderPasses[0].subPasses[0].pipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pc);

				//dibujo la mesh
				vkCmdDrawIndexed(commandBuffers[currFrameDrawn], currPrimitive->indexCount, 1, currPrimitive->indexOffset, currPrimitive->vertexOffset, 0);

				//reset to be drawn variable
				currPrimitive->toBeDrawn = false;
			}
		}
	}
	
	//ligthing subpass
	vkCmdNextSubpass(commandBuffers[currFrameDrawn], VK_SUBPASS_CONTENTS_INLINE);

	//bind pipeline
	vkCmdBindPipeline(commandBuffers[currFrameDrawn], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPasses[0].subPasses[1].pipeline.handle);

	//bind descriptorSets
	vkCmdBindDescriptorSets(commandBuffers[currFrameDrawn], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPasses[0].subPasses[1].pipeline.pipelineLayout, 0, 1, &renderPasses[0].subPasses[1].descriptorSets[currFrameDrawn], 0, nullptr);

	//set dynamic state
	vkCmdSetViewport(commandBuffers[currFrameDrawn], 0, 1, &viewport);
	vkCmdSetScissor(commandBuffers[currFrameDrawn], 0, 1, &tijeras);

	vkCmdDraw(commandBuffers[currFrameDrawn], 4, 1, 0, 0);
	
	//terminar la render Pass
	vkCmdEndRenderPass(commandBuffers[currFrameDrawn]);

	//terminar de grabar el cmdBuffer
	if (vkEndCommandBuffer(commandBuffers[currFrameDrawn]) != VK_SUCCESS) {
		throw std::runtime_error("could not end recording of cmd buffer");
	}

	//submito el frame buffer: semaforo
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currFrameDrawn];

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &gotframeBufferImageSemaforos[currFrameDrawn];//Esperamos al sem�foro 1 en la stage 1 de ambos array, en el sem�foro 2 en la stage 2 y as�
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
	presentInfo.pSwapchains = &swapChain->handle;
	presentInfo.pWaitSemaphores = &imageWrittenSemaforos[currFrameDrawn];
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.swapchainCount = 1;
	presentInfo.pResults = nullptr; //opcional, es un output para checar como le fu� a cada swapChain individualmente

	resultRelatedToSwapChain = vkQueuePresentKHR(presentQueueHandle, &presentInfo);
	if (resultRelatedToSwapChain == VK_ERROR_OUT_OF_DATE_KHR || resultRelatedToSwapChain == VK_SUBOPTIMAL_KHR || windowResized) {
		remakeSwapChain();
		windowResized = false;
	}
	else if (resultRelatedToSwapChain != VK_SUCCESS) {
		throw std::runtime_error("error with queue present");
	}

	//actualizar currFrameDrawn
	currFrameDrawn = (currFrameDrawn + 1) % MAX_FRAMES_IN_FLIGHT;//manera inteligente de loopear al inicio
}

void asgTerminate() {
	vkDeviceWaitIdle(logicalDevice);

	defferedPassFunc::gBufferSubPass::destroyDescriptorSetResources();
	defferedPassFunc::ligthingSubPass::destroyDescriptorSetResources();

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyFence(logicalDevice, frameDrawnFences[i], nullptr);
		vkDestroySemaphore(logicalDevice, imageWrittenSemaforos[i], nullptr);
		vkDestroySemaphore(logicalDevice, gotframeBufferImageSemaforos[i], nullptr);
	}
	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
	vkDestroyCommandPool(logicalDevice, transferCommandPool, nullptr);

	for (auto& renderPass : renderPasses) {
		renderPass.del();
	}

	swapChain->del();//frameBuffers se deben destruir antes de render pass e image views

	for (const auto& pair : materialsBuffers) {
		pair.second->del();
	}

	asgImageHandler::deleteResources();

	vkDestroySurfaceKHR(vulkanInstance, windowSurface, nullptr);//necesita destruirse antes de su instance
	vkDestroyDevice(logicalDevice, nullptr); //ESTO DEBE DE ESTAR ANTES DE DESTROY INSTANCE
	vkDestroyInstance(vulkanInstance, nullptr);//we destroy the instance for cleanup and avoiding leaks

	glfwDestroyWindow(ventana);
	glfwTerminate();
}

//test
//void asgLoadData(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::string materialName) {
//	materialsBuffers.insert(std::make_pair(materialName, std::make_unique<asgVIBuffer>(1000, 1000)));
//	materialsBuffers.at(materialName)->append(vertices, indices);
//}
//void asgLoadDataWithAlbedo(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::string albedoPath, std::string materialName) {
//	materialsBuffers.insert(std::make_pair(materialName, std::make_unique<asgVIBuffer>(1000, 1000)));
//
//	//make model
//	asgPbrIndices pbrIndices;
//	pbrIndices.albedoIndex = asgImageHandler::loadAlbedoMap(albedoPath);
//
//	asgMesh currMesh;
//	asgPrimitive currPrimitive;
//	currPrimitive.pbrIndices = pbrIndices;
//	currPrimitive.matrixIndex = 0;
//	currPrimitive.vertexOffset = materialsBuffers[materialName]->verticesInside;
//	currPrimitive.indexOffset = materialsBuffers[materialName]->indicesInside;
//	currPrimitive.indexCount = static_cast<uint32_t>(indices.size());
//	currPrimitive.toBeDrawn = false;
//
//	asgModel currModel;
//	currModel.path = "";
//	currModel.meshes.push_back(currMesh);
//
//	models.push_back(currModel);
//
//	materialsBuffers.at(materialName)->append(vertices, indices);
//
//	materialsMeshes.insert(std::make_pair(materialName, std::vector<asgMesh*>(0)));
//
//	materialsMeshes[materialName].push_back(&models[models.size() - 1].meshes[models[models.size() - 1].meshes.size() - 1]);
//}
