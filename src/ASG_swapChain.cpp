#include <ASG_swapChain.hpp>

//dependencies
#include <algorithm>//para std::clamp

////*local functions*////
/*declarations*/

/*definitions*/
//helpers
SwapChainSupportDetails getSwapChainSupportDetails(VkPhysicalDevice device) {
	///obtenemos toda la info de la swapchain
	SwapChainSupportDetails swapChainSupportInfo{};

	//surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, windowSurface, &swapChainSupportInfo.capabilities);
	swapChainSupportInfo.capabilities.maxImageCount;
	//surface formats
	uint32_t formatAmount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, windowSurface, &formatAmount, nullptr);
	if (formatAmount != 0) {
		swapChainSupportInfo.formats.resize(formatAmount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, windowSurface, &formatAmount, swapChainSupportInfo.formats.data());
	}

	//surface present modes
	uint32_t presentModesAmount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, windowSurface, &presentModesAmount, nullptr);
	if (presentModesAmount != 0) {
		swapChainSupportInfo.presentModes.resize(presentModesAmount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, windowSurface, &presentModesAmount, swapChainSupportInfo.presentModes.data());
	}

	return swapChainSupportInfo;
}

//main
asgSwapChain::asgSwapChain() {
	//elegimos surface format
	this->supportDetails = getSwapChainSupportDetails(physicalDevice);
		
	surfaceFormat = supportDetails.formats[0];// si no encuetro el que quiero, agarro el primero

	for (const auto& currFormat : supportDetails.formats) {
		if (currFormat.format == VK_FORMAT_B8G8R8A8_SRGB && currFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			surfaceFormat = currFormat;
			if (validationLayersEnabled) {
				printf("format chosen succesfully\n");
			}
			break;
		}
	}

	//elegimos presentation mode
	VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;//default

	for (const auto& currPresentMode : supportDetails.presentModes) {
		if (currPresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			chosenPresentMode = currPresentMode;
		}
	}

	//elegimos swap extent
	swapExtent = supportDetails.capabilities.currentExtent;//default
	if (supportDetails.capabilities.currentExtent.width == (uint32_t)std::numeric_limits<uint32_t>::max()) {
		//conseguimos el tama�o en p�xeles de la ventana
		int pixelWidth, pixelHeigth;
		glfwGetFramebufferSize(ventana, &pixelWidth, &pixelHeigth);
		swapExtent = { static_cast<uint32_t>(pixelWidth), static_cast<uint32_t>(pixelHeigth) };

		//lo clampeamos y lo regresamos
		swapExtent.width = std::clamp(swapExtent.width, supportDetails.capabilities.minImageExtent.width, supportDetails.capabilities.maxImageExtent.width);
		swapExtent.height = std::clamp(swapExtent.height, supportDetails.capabilities.minImageExtent.height, supportDetails.capabilities.maxImageExtent.height);
	}

	//last swapChain detail: image count
	uint32_t swapChainImageCount = 10;//supportDetails.capabilities.minImageCount + 1;//when i changed MAX_FRAMES_IN_FLIGHT but not this, it still ran without errors???

	if (supportDetails.capabilities.maxImageCount != 0 && swapChainImageCount > supportDetails.capabilities.maxImageCount) {
		swapChainImageCount = supportDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = windowSurface;

	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = swapExtent;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.presentMode = chosenPresentMode;
	swapChainCreateInfo.imageArrayLayers = 1; // amount of layers an image has, always one unless developing stereoscopic 3d
	swapChainCreateInfo.minImageCount = swapChainImageCount;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;//we are renndering directly to them so they are color attachment
	//the one you would use for drawing them somewhere else and postprocessing would be VK_IMAGE_USAGE_TRANSFER_DST_BIT

	queueFamilyIndices selectedQueueFamilies = getSelectedQueueFamilies(physicalDevice);

	if (selectedQueueFamilies.graphicsFamilyIndex != selectedQueueFamilies.presentFamilyIndex) {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;//these 2 args specify wich queue families will share the images
		swapChainCreateInfo.pQueueFamilyIndices = selectedQueueFamilies.allFamilyIndices;//this is a bit sketchy bc it assumes the first two are the ones im checking for equality
	}
	else {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;//optional
		swapChainCreateInfo.pQueueFamilyIndices = nullptr; //optional
	}

	swapChainCreateInfo.preTransform = supportDetails.capabilities.currentTransform;//we can specify a transformation to apply to all images, here we are specifying we dont want any
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;//should the alpha be used to blend with other windows in the windows system, almost always no
	swapChainCreateInfo.clipped = VK_TRUE;//clip obscured pixels, like by other windows
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(logicalDevice, &swapChainCreateInfo, nullptr, &handle) != VK_SUCCESS) {
		throw std::runtime_error("could not create swap chain");
	}

	//obtenemos las handles de las imagenes, vulkan puede crear las que quiera as� que hay que pedir cuantas son
	uint32_t amountOfSwapChainImages = 0;
	vkGetSwapchainImagesKHR(logicalDevice, handle, &amountOfSwapChainImages, nullptr);

	if (validationLayersEnabled) {
		printf("\namount of swapChain images: %u\n", amountOfSwapChainImages);
	}

	images.resize(amountOfSwapChainImages);
	views.resize(amountOfSwapChainImages);
	vkGetSwapchainImagesKHR(logicalDevice, handle, &amountOfSwapChainImages, images.data());
	
	if (validationLayersEnabled) {
		printf("before creating image views\n");
	}

	/*create imageViews*/
	for (size_t i = 0; i < images.size(); i++) {
		VkImageViewCreateInfo currImageViewInfo{};
		currImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		currImageViewInfo.image = images[i];

		currImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		currImageViewInfo.format = surfaceFormat.format;
		//estos te permiten mucha customizaci�n de los color channels
		currImageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		currImageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		currImageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		currImageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		currImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;//indicamos que estas im�genes son color targets
		currImageViewInfo.subresourceRange.baseArrayLayer = 0;// no queremos mip maps ni multiple layers
		currImageViewInfo.subresourceRange.baseMipLevel = 0;
		currImageViewInfo.subresourceRange.layerCount = 1;
		currImageViewInfo.subresourceRange.levelCount = 1;
		if (vkCreateImageView(logicalDevice, &currImageViewInfo, nullptr, &views[i]) != VK_SUCCESS) {
			throw std::runtime_error("could not create Image Views");
		}
	}
		
	/*depth buffer*///debe estar debajo de create swap chain y arriba de create framebuffers
	//conseguimos supported format
	bool wasFormatFound = false;
	std::vector<VkFormat> candidates = { VK_FORMAT_D32_SFLOAT_S8_UINT,VK_FORMAT_D24_UNORM_S8_UINT };
	for (VkFormat currFormat : candidates) {
		//conseguimos las propiedades de los formatos que son supported por el physical device
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, currFormat, &formatProperties);
		if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT){
			this->depthBufferFormat = currFormat;
			wasFormatFound = true;
			break;
		}
	}
	if (!wasFormatFound) {
		throw std::runtime_error("Could not find a supported format");
	}
	//imagen	
	VkImageCreateInfo depthImageCI{};
	depthImageCI.arrayLayers = 1;
	depthImageCI.extent = { swapExtent.width, swapExtent.height, 1 };
	depthImageCI.flags = 0;//Opcional par amemory eficiency
	depthImageCI.format = depthBufferFormat;
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

	if (vkCreateImage(logicalDevice, &depthImageCI, nullptr, &depthBuffer) != VK_SUCCESS) {
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
	depthBufferImageViewCI.format = depthBufferFormat;
	depthBufferImageViewCI.image = depthBuffer;
	depthBufferImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

	depthBufferImageViewCI.subresourceRange.baseArrayLayer = 0;
	depthBufferImageViewCI.subresourceRange.baseMipLevel = 0;
	depthBufferImageViewCI.subresourceRange.layerCount = 1;
	depthBufferImageViewCI.subresourceRange.levelCount = 1;
	depthBufferImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

	depthBufferImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthBufferImageViewCI.flags = 0;
	vkCreateImageView(logicalDevice, &depthBufferImageViewCI, nullptr, &depthBufferImageView);

	if (validationLayersEnabled) {
		printf("before returning from swapchain\n");
	}

	//ahora que ya cre� los recursos para el depth buffer debo de cambiar su layout
	transitionImageLayout(depthBuffer, depthBufferFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}
void asgSwapChain::del() {
	for (auto imageView : this->views) {
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	}
	vkDestroyImage(logicalDevice, depthBuffer, nullptr);
	vkDestroyImageView(logicalDevice, depthBufferImageView, nullptr);
	vkFreeMemory(logicalDevice, depthBufferMemory, nullptr);

	vkDestroySwapchainKHR(logicalDevice, handle, nullptr);
}

void waitUntilCanRemakeSwapChain() {
	int currScreenWidth = 0, currScreenHeigth = 0;
	glfwGetFramebufferSize(ventana, &currScreenWidth, &currScreenHeigth);
	while (currScreenWidth == 0 || currScreenHeigth == 0) {//mientras que la ventana mide 0x0 (minimizada), espero
		glfwGetFramebufferSize(ventana, &currScreenWidth, &currScreenHeigth);
		glfwWaitEvents();//espera a que ocurra algo referente a la ventana antes de checar su tama�o otra vez
	}
		
	vkDeviceWaitIdle(logicalDevice);//primero espero a que todas las async op terminen, tecnicamente podr�a recrear la swapChain dandole la anterior en oldSwapChain para que termine de ejecutar las ops de la antigua
}


