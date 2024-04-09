#include <stdio.h>

//#define VK_USE_PLATFORM_WIN32_KHR estos solo se usan si vas a hacer la conexion con una surface tu
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
//#define GLFW_EXPOSE_NATIVE_WIN32
//#include <glfw/glfw3native.h>

#include <fileLoader.hpp>
#include <iostream>
#include <cstddef>
#include <vector>
#include <cstring>
#include <optional>
#include <set>

#define GRAPHICS_FAMILY_PRESENT static_cast<std::byte>(1)
#define PRESENT_FAMILY_PRESENT static_cast<std::byte>(2)
#define ALL_FAMILIES_PRESENT static_cast<std::byte>(3)

/*window surface*/
VkSurfaceKHR windowSurface;

/*variables de extensiones*/
const std::vector<const char*> usedExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};//es un typedef string

/*variables de validation layers*/
const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};


#ifdef NDEBUG
	const bool validationLayersEnabled = false;
#else
	const bool validationLayersEnabled = true;
#endif

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

bool isPhysicalDeviceSuitable(VkPhysicalDevice device){
	queueFamilyIndices selectedQueueFamilies = getSelectedQueueFamilies(device);

	return (selectedQueueFamilies.hasAllRequirements() && checkDeviceExtensionSupport(device));
}

int main() {
	printf("AAAAA");
	std::cout << "validation layers enabled: " << validationLayersEnabled << std::endl;
	/*window creation*/
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);//como es vulkan tenemos que hacer algo especial para rezizable windows
	GLFWwindow* ventana = glfwCreateWindow(500,500,"uno",NULL,NULL);

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
	for(uint32_t familyIndex : selectedQueueFamilies.allFamilyIndices){
		VkDeviceQueueCreateInfo queueCreateInfo{};

		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pQueuePriorities = &allQueuePriority;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = familyIndex;
		
		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	
	//especificamos las features que se usarán
	VkPhysicalDeviceFeatures deviceFeaturesUsed{};//ninguna en especial así que dejamos todo en VK_FALSE
	printf("\nsize of queueCreateInfos vector: %u\n", static_cast<uint32_t>(queueCreateInfos.size()));
	///hacemos createinfo
	VkDeviceCreateInfo logicalDeviceCreateInfo{};
	logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
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
	
	//checar las extensiones que tenemos ps nomás
	uint32_t extCount = 0;

	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	std::vector<VkExtensionProperties> extProperties(extCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extProperties.data());

	printf("\n\nExtensiones\n");
	for (int i = 0; i < extCount; i++){
		printf("\n\t %s \n", extProperties[i].extensionName);
	}



	//read program data
	/*
	std::vector <unsigned char> vertexData, fragmentData;
	readBinFile("./testProgram.frag.spv\n", &fragmentData);
	std::cout << "\nfragment data size: " << fragmentData.size() << std::endl;

	readBinFile("./testProgram.vert.spv\n", &vertexData);
	std::cout << "\nvertex data size: " << vertexData.size() << std::endl;
	printf("AAA");
	*/

	/*main loop*/
	while (!glfwWindowShouldClose(ventana)) {
		glfwPollEvents();
	}

	for (auto familyIndex : selectedQueueFamilies.allFamilyIndices){
		printf("curr family: %u\n", familyIndex);
	}
	printf("are graphics and present family equal: %d", presentQueueHandle == graphicsQueueHandle);
	vkDestroySurfaceKHR(vulkanInstance, windowSurface, nullptr);//necesita destruirse antes de su instance
	vkDestroyDevice(logicalDevice, nullptr); //ESTO DEBE DE ESTAR ANTES DE DESTROY INSTANCE
	vkDestroyInstance(vulkanInstance, nullptr);//we destroy the instance for cleanup and avoiding leaks

	glfwDestroyWindow(ventana);
	glfwTerminate();
	
	return 0;
}