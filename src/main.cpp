#define NOMINMAX //para que windows no defina max y me joda el numeric_limits
#include <stdio.h>

//#define VK_USE_PLATFORM_WIN32_KHR // estos solo se usan si vas a hacer la conexion con una surface tu
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
//#define GLFW_EXPOSE_NATIVE_WIN32
//#include <glfw/glfw3native.h>

#include <fileLoader.hpp>
#include <binFileLoader.hpp>
#include <iostream>
#include <cstddef>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>

/*definitions echas por mi*/
#define GRAPHICS_FAMILY_PRESENT static_cast<std::byte>(1)
#define PRESENT_FAMILY_PRESENT static_cast<std::byte>(2)
#define ALL_FAMILIES_PRESENT static_cast<std::byte>(3)

#define SCREENWIDTH 500
#define SCREENHEIGTH 500

/*globales*/
VkSurfaceKHR windowSurface;

const std::vector<const char*> usedExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};//es un typedef string
const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

std::vector<VkImage> swapChainImages;
std::vector<VkImageView> swapChainImageViews;


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
			};
		uint32_t allFamilyIndices[2];
	};

	std::byte familiesPresentBitMask = static_cast<std::byte>(0);

	bool hasAllRequirements(){//generic check
		return (familiesPresentBitMask == ALL_FAMILIES_PRESENT);
	}
};

VkShaderModule createShaderModule(std::vector<unsigned char> rawDataVector, VkDevice logicalDevice){
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

/*funciones necesarias para is Physical device suitable*/
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
		if(queueFamiliesProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
			selectedQueueFamilies.graphicsFamilyIndex = i;
			selectedQueueFamilies.familiesPresentBitMask |= GRAPHICS_FAMILY_PRESENT;
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

bool isPhysicalDeviceSuitable(VkPhysicalDevice device){
	queueFamilyIndices selectedQueueFamilies = getSelectedQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool swapChainHasAllRequirements = false;
	if (extensionsSupported){//debemos checar la swap chain solo si ya nos aseguramos que su extensión si tiene support
		SwapChainSupportDetails swapChainDetails = getSwapChainSupportDetails(device);
		swapChainHasAllRequirements = !swapChainDetails.formats.empty() && ! swapChainDetails.presentModes.empty();
	}

	return (selectedQueueFamilies.hasAllRequirements() && extensionsSupported && swapChainHasAllRequirements);//ponemos extensions supported por completness
}

int main() {
	printf("AAAAA");
	std::cout << "validation layers enabled: " << validationLayersEnabled << std::endl;
	/*window creation*/
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);//como es vulkan tenemos que hacer algo especial para rezizable windows
	GLFWwindow* ventana = glfwCreateWindow(SCREENWIDTH,SCREENHEIGTH,"uno",NULL,NULL);

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
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

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
	
	VkDevice logicalDevice{};

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
	VkQueue graphicsQueueHandle, presentQueueHandle;
	vkGetDeviceQueue(logicalDevice, selectedQueueFamilies.graphicsFamilyIndex, 0, &graphicsQueueHandle); // en queue index va su indice de queue de esta familia, solo tenemos uno de cada familia así que es 0 en todos.
	vkGetDeviceQueue(logicalDevice, selectedQueueFamilies.presentFamilyIndex, 0, &presentQueueHandle); 

	/*creamos la swap chain*/
	//elegimos surface format
	SwapChainSupportDetails swapChainInfo = getSwapChainSupportDetails(physicalDevice);
	VkSurfaceFormatKHR chosenSwapSurfaceFormat = swapChainInfo.formats[0];// si no encuetro el que quiero, agarro el primero

	for (const auto& currFormat: swapChainInfo.formats){
		if(currFormat.format == VK_FORMAT_B8G8R8A8_SRGB && currFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
			chosenSwapSurfaceFormat = currFormat;
			printf("format chosen succesfully\n");
			break;
		}
	}

	//elegimos presentation mode
	VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;//default

	for (const auto& currPresentMode : swapChainInfo.presentModes){
		if (currPresentMode == VK_PRESENT_MODE_MAILBOX_KHR){
			chosenPresentMode = currPresentMode;
		}
	}

	//elegimos swap extent
	VkExtent2D chosenSwapExtent = swapChainInfo.capabilities.currentExtent;//default
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

	VkSwapchainKHR swapChain;

	if(vkCreateSwapchainKHR(logicalDevice, &swapChainCreateInfo, nullptr, &swapChain) != VK_SUCCESS){
		throw std::runtime_error("could not create swap chain");
	}else{
		printf("swap chain created");
	}

	//obtenemos las handles de las imagenes, vulkan puede crear las que quiera así que hay que pedir cuantas son
	uint32_t amountOfSwapChainImages = 0;
	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &amountOfSwapChainImages, nullptr);

	swapChainImages.resize(amountOfSwapChainImages);
	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &amountOfSwapChainImages, swapChainImages.data());

	/*image views*/
	swapChainImageViews.resize(swapChainImages.size());

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
			printf("Image views created");
		}
	}
	printf("\n");

	//checar las extensiones que tenemos ps nomás
	uint32_t extCount = 0;

	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	std::vector<VkExtensionProperties> extProperties(extCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extProperties.data());

	//printf("\n\nExtensiones\n");
	for (int i = 0; i < extCount; i++){
	//	printf("\n\t %s \n", extProperties[i].extensionName);
	}

	/*graphics pipeline*/
	//read program data
	std::vector<unsigned char> vertexData = readRawBinary("./testProgram.vert.spv");
	std::cout << "\nfragment data size: " << vertexData.size() << std::endl;

	std::vector<unsigned char> fragmentData = readRawBinary("./testProgram.frag.spv");
	std::cout << "\nvertex data size: " << vertexData.size() << std::endl;

	//create shader modules
	VkShaderModule vertexShaderModule = createShaderModule(vertexData, logicalDevice);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentData, logicalDevice);

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
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;

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
	rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
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
	VkPipelineLayout pipelineLayout;
	VkPipelineLayoutCreateInfo pipelineLayoutCI{};
	pipelineLayoutCI.pPushConstantRanges = nullptr;
	pipelineLayoutCI.pSetLayouts = nullptr;
	pipelineLayoutCI.pushConstantRangeCount = 0;
	pipelineLayoutCI.setLayoutCount = 0;
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
	colorAttachmentReference.attachment = 0;//indice, el mismo de layout(location = 0)
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;//el layout con el que trataré ese attachment, en este caso como color buffer

	//cada subpass necesita descripción
	VkSubpassDescription colorSubpassDescription{};
	colorSubpassDescription.colorAttachmentCount = 1;
	colorSubpassDescription.pColorAttachments = &colorAttachmentReference;
	colorSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;//esta subpass es de graficos
	
	//creamos renderPass
	VkRenderPass renderPass;
	VkRenderPassCreateInfo renderPassCI{};
	renderPassCI.attachmentCount = 1;
	renderPassCI.pAttachments = &colorAttachment;
	renderPassCI.pSubpasses = &colorSubpassDescription;
	renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCI.subpassCount = 1;

	if(vkCreateRenderPass(logicalDevice, &renderPassCI, nullptr, &renderPass) != VK_SUCCESS){
		throw std::runtime_error("could not create render pass");
	}

	//ya crear la pipeline
	VkPipeline graphicsPipeline;
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
	graphicsPipelineCI.pDepthStencilState = nullptr;
	graphicsPipelineCI.pMultisampleState = &multisampleCreateInfo;
	graphicsPipelineCI.pRasterizationState = &rasterizationCreateInfo;
	graphicsPipelineCI.subpass = 0;//indice

	if(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, &graphicsPipeline) != VK_SUCCESS){
		throw std::runtime_error("could not create graphics pipeline");
	}

	

	//Destroy shader modules as soon as the code is in te pipeline just like openGL
	vkDestroyShaderModule(logicalDevice, vertexShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragmentShaderModule, nullptr);

	/*main loop*/
	while (!glfwWindowShouldClose(ventana)) {
		glfwPollEvents();
	}

	for (auto familyIndex : selectedQueueFamilies.allFamilyIndices){
		printf("curr family: %u\n", familyIndex);
	}
	printf("are graphics and present family equal: %d", presentQueueHandle == graphicsQueueHandle);

	for (auto imageView : swapChainImageViews){
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	}

	vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
	vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
	vkDestroySwapchainKHR(logicalDevice,swapChain,nullptr);
	vkDestroySurfaceKHR(vulkanInstance, windowSurface, nullptr);//necesita destruirse antes de su instance
	vkDestroyDevice(logicalDevice, nullptr); //ESTO DEBE DE ESTAR ANTES DE DESTROY INSTANCE
	vkDestroyInstance(vulkanInstance, nullptr);//we destroy the instance for cleanup and avoiding leaks

	glfwDestroyWindow(ventana);
	glfwTerminate();
	
	return 0;
}