//builtin
#include <memory>
#include <map>

//dependencies
#include <STB/stb_image.h>
#include <sha256.h>

//local
#include <ASG_imageHandler.hpp>
#include <ASG_renderPass_deffered.hpp>

//variables
std::vector<std::string> loadedImagesPathsAndDigests;//los hiba a poner por separado pero al final ambos era string y se manejaban igual
std::vector<asgDeviceMemory> memories;
uint32_t numOfMaterialsLoaded;

std::vector<std::pair<std::string, VkImage>> albedoMaps;//estos pueden ser más que el número de materiales cargados
std::vector<VkImageView> albedoImageViews;//si necesito conseguir la imagen relacionada al imageView podria considerar un mapa
VkSampler defaultAlbedoSampler;

//std::unique_ptr<descriptorSetImageUpdater> dsImageUpdater;

SHA256 sha256;

//helpers
bool isImageAlreadyLoaded(std::string pathOrDigest) {
	for (std::string currPath : loadedImagesPathsAndDigests) {
		if (currPath == pathOrDigest) {
			if (validationLayersEnabled) {
				printf("tried to load already loaded image");
			}
			return true;
		}
	}

	return false;
}
asgDeviceMemory* getRightMemory(VkMemoryRequirements memoryRequirements) {
	uint32_t rightMemoryTypeIndex = findRigthMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	for (int i = 0; i < memories.size(); i++) {
		if (memories[i].memoryTypeIndex == rightMemoryTypeIndex) {
			return &memories[i];
		}
	}

	//si no la encuentra, la alojamos
	VkMemoryAllocateInfo memoryAI{};
	VkDeviceMemory currMemory;
	uint32_t allocationSize = 50000000;

	while (allocationSize < memoryRequirements.size) {
		allocationSize *= 2;
	}

	memoryAI.allocationSize = allocationSize;//alojo la cantidad que me dice que debo alojar, no el tamaño real
	memoryAI.memoryTypeIndex = rightMemoryTypeIndex;
	memoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	if (vkAllocateMemory(logicalDevice, &memoryAI, nullptr, &currMemory) != VK_SUCCESS) {
		throw std::runtime_error("Could not allocate image memory");
	}

	asgDeviceMemory newMemory;
	newMemory.handle = currMemory;
	newMemory.memoryTypeIndex = rightMemoryTypeIndex;
	newMemory.ocupiedBytes = 0;
	newMemory.size = allocationSize;

	memories.push_back(newMemory);
	return &memories[memories.size() - 1];
}
void addAlbedoMap(VkImageView albedoMapImageView) {
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorImageInfo currII{};//tengo el sampler y la textura en el descriptor set de los uniforms, ndmas en otro binding
		currII.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;//en este lo tengo en createTextureImage
		currII.imageView = albedoMapImageView;
		currII.sampler = defaultAlbedoSampler;

		VkWriteDescriptorSet currWriteDescriptorSets{};//No funcionaba con arrays de c???
		currWriteDescriptorSets.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		currWriteDescriptorSets.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		currWriteDescriptorSets.dstArrayElement = defferedPassFunc::gBufferSubPass::amountOfLoadedAlbedos;
		currWriteDescriptorSets.descriptorCount = 1;//no need for arbitrary size because i only update one at a time
		currWriteDescriptorSets.dstSet = renderPasses[0].subPasses[0].descriptorSets[i];//TODO DEBUG cambie de 1 a 0 el indice de subPasses[ESTE]

		currWriteDescriptorSets.dstBinding = 1;

		currWriteDescriptorSets.pImageInfo = &currII;//no es necesario poner los otros en nullptr?
		currWriteDescriptorSets.pBufferInfo = nullptr;
		currWriteDescriptorSets.pTexelBufferView = nullptr;
		vkUpdateDescriptorSets(logicalDevice, 1, &currWriteDescriptorSets, 0, nullptr);// lo de copy descriptors es ps para copiarlos
	}

	defferedPassFunc::gBufferSubPass::amountOfLoadedAlbedos++;
}

//func
void asgImageHandler::initializeResources() {//initialize atributes, doing so outside of a function would fail bc it would initialize before asgInit
	numOfMaterialsLoaded = 0;
	
	//Sampler
	VkSamplerCreateInfo testSamplerCI{};
	testSamplerCI.unnormalizedCoordinates = VK_FALSE;//para que use [0-1) en vez de [0,imageLong)
	testSamplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	testSamplerCI.compareEnable = VK_FALSE;
	testSamplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	testSamplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	testSamplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	testSamplerCI.magFilter = VK_FILTER_LINEAR;//para que no se vea blocky
	testSamplerCI.minFilter = VK_FILTER_LINEAR;
	
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	testSamplerCI.anisotropyEnable = VK_TRUE;
	testSamplerCI.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;

	testSamplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;//solo se usa con clamp to border
	testSamplerCI.compareOp = VK_COMPARE_OP_ALWAYS;

	testSamplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;//estos 4 son de mipmaping
	testSamplerCI.mipLodBias = 0.0f;
	testSamplerCI.minLod = 0.0f;
	testSamplerCI.maxLod = 0.0f;
	if (vkCreateSampler(logicalDevice, &testSamplerCI, nullptr, &defaultAlbedoSampler) != VK_SUCCESS) {
		throw std::runtime_error("could not create test sampler");
	}

}

uint32_t asgImageHandler::loadAlbedoMap(std::string path) {
	//si la imagen ya ha sido cargada regresar
		//usa isImageAlreadyLoaded
	//cargar la imagen
	//crear vkImage
	// conectarle memoria
		//conseguir una memoria de la lista de memorias que tengo
			//busca la memoria con el memoryTypeIndex requerido
				//si la encuentra la regresa
				// si no, la aloja
			// ahora que tiene la memoria, al conecta en un offset calculado con asgMemory.occupiedBytes
	//meter la imagen a staging buffer
	//liberar la imagen
	//pasar la memoria del stagingBuffer a la imagen
	//albedoMaps.append currImagen
	//createImageView
	//albedoImageViews.append currImageView

	if (isImageAlreadyLoaded(path)) {
		uint32_t index = 0;
		for (const auto& pair : albedoMaps) {
			if (path == pair.first) {
				return index;
			}
			index++;
		}
		throw std::runtime_error("it said its already loaded but couldnt find its index???");
	}
	else {
		loadedImagesPathsAndDigests.push_back(path);
	}

	//load image
	int imageLong, imageTall, numCollChannels;//de momento no uso numCollChannels
	stbi_uc *rawData = stbi_load(path.data(), &imageLong, &imageTall, &numCollChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = imageLong * imageTall * 4;
	
	if (!rawData) {
		printf(stbi_failure_reason());
		throw std::runtime_error("could not load image\n");
	}

	//create vkImage
	VkImage currImage;
	VkImageCreateInfo imageCI{};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.arrayLayers = 1;
	imageCI.imageType = VK_IMAGE_TYPE_2D;//dice que tipo de coordenadas usará
	imageCI.extent.width = static_cast<uint32_t>(imageLong);
	imageCI.extent.height = static_cast<uint32_t>(imageTall);
	imageCI.extent.depth = 1;
	imageCI.mipLevels = 1;
	imageCI.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;//si fuera LINEAR ordenaria los pixeles como ps una matriz, OPTIMAL es óptimo, pero su ordenamiento es implementation defined
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;//UNDEFINED significa que le permitimos a la primera transición descartar pixeles
	imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.flags = 0;//Puede usarse para memory eficiency

	if (vkCreateImage(logicalDevice, &imageCI, nullptr, &currImage) != VK_SUCCESS) {
		throw std::runtime_error("could not create vkImage");
	}
	
	//bind Memory
	VkMemoryRequirements currImageMemoryRequirements;
	vkGetImageMemoryRequirements(logicalDevice, currImage, &currImageMemoryRequirements);

	asgDeviceMemory *rightMemory = getRightMemory(currImageMemoryRequirements);
	vkBindImageMemory(logicalDevice, currImage, rightMemory->handle, currImageMemoryRequirements.alignment * static_cast<uint32_t>(ceil(rightMemory->ocupiedBytes/ currImageMemoryRequirements.alignment)));
	rightMemory->ocupiedBytes = static_cast<uint32_t>(currImageMemoryRequirements.alignment * ceil(rightMemory->ocupiedBytes / currImageMemoryRequirements.alignment) + currImageMemoryRequirements.size); // occupied = offset + size

	//meter la imagen a staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	createMemoryIndependentBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingMemory);//notese que uso imageSize y no memReq.size

	void* stagingData;
	vkMapMemory(logicalDevice, stagingMemory, 0, VK_WHOLE_SIZE, 0, &stagingData);
	memcpy(stagingData, rawData, imageSize);
	vkUnmapMemory(logicalDevice, stagingMemory);
	
	stbi_image_free(rawData);

	//pasar la memoria del stagingBuffer a la imagen
	transitionImageLayout(currImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkCommandBuffer cmdBuffer = createSingleUseCmdBuffer();
	VkBufferImageCopy region{};
	region.bufferImageHeight = 0;//podría ser que en el bufer existieran padding bytes entre los renglones, pero no padding, así que 0 en este y rowLength
	region.bufferOffset = 0;//byte offset into the buffer
	region.bufferRowLength = 0;

	region.imageExtent = { static_cast<uint32_t>(imageLong), static_cast<uint32_t>(imageTall), 1 };
	region.imageOffset = { 0,0,0 };
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.mipLevel = 0;

	vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, currImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);//asumimos que el layout es ese pq ps se supone q es
	endSingleUseCmdBuffer(cmdBuffer, graphicsQueueHandle);
	
	transitionImageLayout(currImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	//añadir imagen al albedomap
	albedoMaps.push_back(std::make_pair(path,currImage));

	//createImageView
	VkImageViewCreateInfo currImageViewCI{};
	currImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	currImageViewCI.image = currImage;
	currImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	currImageViewCI.format = VK_FORMAT_R8G8B8A8_SRGB;
	
	currImageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	currImageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	currImageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	currImageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	currImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;//indicamos que esta imágen es color target
	currImageViewCI.subresourceRange.baseArrayLayer = 0;// no queremos mip maps ni multiple layers
	currImageViewCI.subresourceRange.baseMipLevel = 0;
	currImageViewCI.subresourceRange.layerCount = 1;
	currImageViewCI.subresourceRange.levelCount = 1;

	VkImageView currImageView;
	if (vkCreateImageView(logicalDevice, &currImageViewCI, nullptr, &currImageView) != VK_SUCCESS) {
		throw std::runtime_error("could not create test image image view");
	}
	
	albedoImageViews.push_back(currImageView);

	addAlbedoMap(albedoImageViews[albedoImageViews.size() - 1]);

	//cleanup
	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingMemory, nullptr);

	return static_cast<uint32_t>(albedoMaps.size() - 1);
}

uint32_t asgImageHandler::loadAlbedoMap(std::vector<unsigned char> data, uint32_t imageTall, uint32_t imageLong, uint32_t  numCollChannells) {
	//de momento no uso numCollChannels
	//checar si la imagen ya ha sido cargada
	std::string digest = sha256(data.data(), data.size());
	if (isImageAlreadyLoaded(digest)) {
		uint32_t index = 0;
		for (const auto& pair : albedoMaps) {
			if (digest == pair.first) {
				return index;
			}
			index++;
		}
		throw std::runtime_error("it said its already loaded but couldnt find its index???");
	}
	else {
		loadedImagesPathsAndDigests.push_back(digest);
	}

	//create vkImage
	VkDeviceSize imageSize = imageLong * imageTall * 4;

	VkImage currImage;
	VkImageCreateInfo imageCI{};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.arrayLayers = 1;
	imageCI.imageType = VK_IMAGE_TYPE_2D;//dice que tipo de coordenadas usará
	imageCI.extent.width = static_cast<uint32_t>(imageLong);
	imageCI.extent.height = static_cast<uint32_t>(imageTall);
	imageCI.extent.depth = 1;
	imageCI.mipLevels = 1;
	imageCI.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;//si fuera LINEAR ordenaria los pixeles como ps una matriz, OPTIMAL es óptimo, pero su ordenamiento es implementation defined
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;//UNDEFINED significa que le permitimos a la primera transición descartar pixeles
	imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.flags = 0;//Puede usarse para memory eficiency

	if (vkCreateImage(logicalDevice, &imageCI, nullptr, &currImage) != VK_SUCCESS) {
		throw std::runtime_error("could not create vkImage");
	}

	//bind Memory
	VkMemoryRequirements currImageMemoryRequirements;
	vkGetImageMemoryRequirements(logicalDevice, currImage, &currImageMemoryRequirements);

	asgDeviceMemory* rightMemory = getRightMemory(currImageMemoryRequirements);
	vkBindImageMemory(logicalDevice, currImage, rightMemory->handle, currImageMemoryRequirements.alignment * static_cast<uint32_t>(ceil(rightMemory->ocupiedBytes / currImageMemoryRequirements.alignment)));
	rightMemory->ocupiedBytes = static_cast<uint32_t>(currImageMemoryRequirements.alignment * ceil(rightMemory->ocupiedBytes / currImageMemoryRequirements.alignment) + currImageMemoryRequirements.size); // occupied = offset + size

	//meter la imagen a staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	createMemoryIndependentBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingMemory);//notese que uso imageSize y no memReq.size

	void* stagingData;
	vkMapMemory(logicalDevice, stagingMemory, 0, VK_WHOLE_SIZE, 0, &stagingData);
	memcpy(stagingData, data.data(), imageSize);
	vkUnmapMemory(logicalDevice, stagingMemory);

	//pasar la memoria del stagingBuffer a la imagen
	transitionImageLayout(currImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkCommandBuffer cmdBuffer = createSingleUseCmdBuffer();
	VkBufferImageCopy region{};
	region.bufferImageHeight = 0;//podría ser que en el bufer existieran padding bytes entre los renglones, pero no padding, así que 0 en este y rowLength
	region.bufferOffset = 0;//byte offset into the buffer
	region.bufferRowLength = 0;

	region.imageExtent = { static_cast<uint32_t>(imageLong), static_cast<uint32_t>(imageTall), 1 };
	region.imageOffset = { 0,0,0 };
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.mipLevel = 0;

	vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, currImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);//asumimos que el layout es ese pq ps se supone q es
	endSingleUseCmdBuffer(cmdBuffer, graphicsQueueHandle);

	transitionImageLayout(currImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	//añadir imagen al albedomap
	albedoMaps.push_back(std::make_pair(digest, currImage));

	//createImageView
	VkImageViewCreateInfo currImageViewCI{};
	currImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	currImageViewCI.image = currImage;
	currImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	currImageViewCI.format = VK_FORMAT_R8G8B8A8_SRGB;

	currImageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	currImageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	currImageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	currImageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	currImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;//indicamos que esta imágen es color target
	currImageViewCI.subresourceRange.baseArrayLayer = 0;// no queremos mip maps ni multiple layers
	currImageViewCI.subresourceRange.baseMipLevel = 0;
	currImageViewCI.subresourceRange.layerCount = 1;
	currImageViewCI.subresourceRange.levelCount = 1;

	VkImageView currImageView;
	if (vkCreateImageView(logicalDevice, &currImageViewCI, nullptr, &currImageView) != VK_SUCCESS) {
		throw std::runtime_error("could not create test image image view");
	}

	albedoImageViews.push_back(currImageView);

	addAlbedoMap(albedoImageViews[albedoImageViews.size() - 1]);

	//cleanup
	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingMemory, nullptr);

	return static_cast<uint32_t>(albedoMaps.size() - 1);
}

void asgImageHandler::deleteResources() {
	for (VkImageView currImageView : albedoImageViews) {
		vkDestroyImageView(logicalDevice, currImageView, nullptr);
	}

	for (auto& currPair: albedoMaps) {
		vkDestroyImage(logicalDevice, currPair.second, nullptr);
	}

	for (asgDeviceMemory currMemory : memories) {
		vkFreeMemory(logicalDevice, currMemory.handle, nullptr);
	}

	vkDestroySampler(logicalDevice, defaultAlbedoSampler, nullptr);
}