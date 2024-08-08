//dependencies
#include <STB/stb_image.h>

//builtin
#include <vector>
#include <Array>

//local
#include <ASG_descriptorSets.hpp>
#include <ASG_utils.hpp>
#include <ASG_graphicsPipeline.hpp>

//////////////////*Var*/
//descriptors
VkDescriptorPool descriptorPool;
std::vector<VkDescriptorSet> descriptorSets(MAX_FRAMES_IN_FLIGHT);

//uniforms, las puse aqui pq son como subtema de descriptors
std::vector<VkBuffer>uniformBuffers(MAX_FRAMES_IN_FLIGHT);//estoy separando los buffers principalmente por tipo de memoria
VkDeviceMemory matrixUniformMemory;
std::vector<void*> mappedUniformBufferMemories(MAX_FRAMES_IN_FLIGHT);

std::vector<VkBuffer>meshModelMatricesBuffers(MAX_FRAMES_IN_FLIGHT);
VkDeviceMemory meshModelMatricesMemory;
std::vector<void*> mappedMeshModelMatricesBuffers(MAX_FRAMES_IN_FLIGHT);

uint32_t amountOfLoadedAlbedos = 0;//hiba a pensar en como lidiar con unloading de imageViews, pero nada del código lidia con eso así q ps así lo dejo y cuando lidie con unloading lo hago todo junto 

//constant uniforms
VkDeviceMemory constantUniformsMemory;

//lightingThresholds variables
VkImage lightingThresholds;
VkImageView lightingThresholdsIV;
VkSampler lightingThresholdsSampler;

///////helpers
VkBuffer createMeshModelBuffer() {
	//llenar el struct (todos tienen la misma información)
	VkBufferCreateInfo returnBufferCI{};
	returnBufferCI.pQueueFamilyIndices = nullptr;
	returnBufferCI.queueFamilyIndexCount = 1;
	returnBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	returnBufferCI.size = 50 * 64;//TODO 50 arbitrario, 64 es el tamaño de una mat4
	returnBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	returnBufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	//mandarlo a la función (lo mismo)
	VkBuffer returnBuffer;
	if (vkCreateBuffer(logicalDevice, &returnBufferCI, nullptr, &returnBuffer) != VK_SUCCESS) {
		throw std::runtime_error("could not createBuffer");
	}
	return returnBuffer;
}

//////////////////*Func*/
void initializeDescriptorSets(asgPipeline* graphicsPipeline) {
	/*createUniformBuffers*/
	std::vector<VkMemoryRequirements> uniformBuffersMemoryRequirements(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkBufferCreateInfo currUniformBufferCI{};
		currUniformBufferCI.pQueueFamilyIndices = nullptr;
		currUniformBufferCI.queueFamilyIndexCount = 1;
		currUniformBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		currUniformBufferCI.size = sizeof(MatrixTransformations);
		currUniformBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		currUniformBufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		if (vkCreateBuffer(logicalDevice, &currUniformBufferCI, nullptr, &uniformBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("could not createBuffer");
		}
		vkGetBufferMemoryRequirements(logicalDevice, uniformBuffers[i], &uniformBuffersMemoryRequirements[i]);//como son iguales todos los buffers, se supone que los memReq son ig para todos, pero por si acaso los checo
	}

	matrixUniformMemory = allocateDeviceMemory(uniformBuffersMemoryRequirements);
	bindMultipleMappedBuffers(uniformBuffersMemoryRequirements, uniformBuffers, &mappedUniformBufferMemories, matrixUniformMemory);

	/*create buffers for meshModelMatrices*/
	//crear los buffers
	for (int i = 0; i < meshModelMatricesBuffers.size(); i++) {
		meshModelMatricesBuffers[i] = createMeshModelBuffer();
	}

	//creamos el vector de memRequirements
	std::vector<VkMemoryRequirements> meshBuffersMemoryRequirements(meshModelMatricesBuffers.size());
	for (int i = 0; i < meshModelMatricesBuffers.size(); i++) {
		vkGetBufferMemoryRequirements(logicalDevice, meshModelMatricesBuffers[i], &meshBuffersMemoryRequirements[i]);
	}

	meshModelMatricesMemory = allocateDeviceMemory(meshBuffersMemoryRequirements);
	bindMultipleMappedBuffers(meshBuffersMemoryRequirements, meshModelMatricesBuffers, &mappedMeshModelMatricesBuffers, meshModelMatricesMemory);

	/*lightingh thresholds*/
	//load image with stb as single color channel
	int lightingThresholdsTall, lightingThresholdsLong, lightingThresholdsNumCollChannels;
	stbi_uc* lightingThresholdsData = stbi_load("./resourceFiles/nonModelImages/lighting_thresholds.png", &lightingThresholdsLong, &lightingThresholdsTall, &lightingThresholdsNumCollChannels, STBI_grey);

	//create staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createMemoryIndependentBuffer(lightingThresholdsTall * lightingThresholdsLong, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	//put data into staging buffer
	void* mappedStagingBuffer;
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, lightingThresholdsLong * lightingThresholdsTall, 0, &mappedStagingBuffer);

	memcpy(mappedStagingBuffer, lightingThresholdsData, lightingThresholdsTall * lightingThresholdsLong);

	//create vkImage
	VkImageCreateInfo lightingThresholdsCI{};
	lightingThresholdsCI.arrayLayers = 1;
	lightingThresholdsCI.extent = { static_cast<uint32_t>(lightingThresholdsLong), static_cast<uint32_t>(lightingThresholdsTall), 1 };
	lightingThresholdsCI.format = VK_FORMAT_R8_UNORM;
	lightingThresholdsCI.imageType = VK_IMAGE_TYPE_2D;
	lightingThresholdsCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	lightingThresholdsCI.mipLevels = 1;
	queueFamilyIndices  qfi = getSelectedQueueFamilies(physicalDevice);
	lightingThresholdsCI.pQueueFamilyIndices = &qfi.graphicsFamilyIndex;
	lightingThresholdsCI.queueFamilyIndexCount = 1;
	lightingThresholdsCI.samples = VK_SAMPLE_COUNT_1_BIT;
	lightingThresholdsCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	lightingThresholdsCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	lightingThresholdsCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	lightingThresholdsCI.tiling = VK_IMAGE_TILING_OPTIMAL;

	if (vkCreateImage(logicalDevice, &lightingThresholdsCI, nullptr, &lightingThresholds) != VK_SUCCESS) {
		throw std::runtime_error("could not create the image for lighting thresholds");
	}

	//allocate memory
	std::vector<VkMemoryRequirements> constantUniformsMemoryRequirements(1);
	vkGetImageMemoryRequirements(logicalDevice, lightingThresholds, &constantUniformsMemoryRequirements[0]);

	constantUniformsMemory = allocateDeviceMemory(constantUniformsMemoryRequirements);

	//bind it
	vkBindImageMemory(logicalDevice, lightingThresholds, constantUniformsMemory, 0);//TODO memory offset

	//move data to vkImage
	transitionImageLayout(lightingThresholds, VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkCommandBuffer cmdBuffer = createSingleUseCmdBuffer();
	VkBufferImageCopy region{};
	region.bufferImageHeight = 0;//podría ser que en el bufer existieran padding bytes entre los renglones, pero no padding, así que 0 en este y rowLength
	region.bufferOffset = 0;
	region.bufferRowLength = 0;

	region.imageExtent = { static_cast<uint32_t>(lightingThresholdsLong), static_cast<uint32_t>(lightingThresholdsTall), 1 };
	region.imageOffset = { 0,0,0 };
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.mipLevel = 0;

	vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, lightingThresholds, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);//asumimos que el layout es ese pq ps se supone q es
	endSingleUseCmdBuffer(cmdBuffer, graphicsQueueHandle);

	transitionImageLayout(lightingThresholds, VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	//destroy staging stuff
	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

	//create the sampler
	VkSamplerCreateInfo lightingThresholdsSamplerCI{};
	lightingThresholdsSamplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	lightingThresholdsSamplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	lightingThresholdsSamplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	lightingThresholdsSamplerCI.anisotropyEnable = VK_FALSE;
	lightingThresholdsSamplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	lightingThresholdsSamplerCI.compareEnable = VK_FALSE;
	lightingThresholdsSamplerCI.compareOp = VK_COMPARE_OP_ALWAYS;
	lightingThresholdsSamplerCI.magFilter = VK_FILTER_NEAREST;
	lightingThresholdsSamplerCI.maxAnisotropy = 1.0f;
	lightingThresholdsSamplerCI.maxLod = 1.0f;
	lightingThresholdsSamplerCI.minFilter = VK_FILTER_LINEAR;
	lightingThresholdsSamplerCI.minLod = 0.0f;
	lightingThresholdsSamplerCI.mipLodBias = 0.0f;
	lightingThresholdsSamplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	lightingThresholdsSamplerCI.unnormalizedCoordinates = VK_FALSE;
	lightingThresholdsSamplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	if (vkCreateSampler(logicalDevice, &lightingThresholdsSamplerCI, nullptr, &lightingThresholdsSampler) != VK_SUCCESS) {
		throw std::runtime_error("could not create lighting thresholds sampler");
	}
	
	//crear la image view
	VkImageViewCreateInfo lightingThresholdsIVCI{};
	lightingThresholdsIVCI.components.r = VK_COMPONENT_SWIZZLE_R;
	lightingThresholdsIVCI.components.g = VK_COMPONENT_SWIZZLE_G;
	lightingThresholdsIVCI.components.b = VK_COMPONENT_SWIZZLE_B;
	lightingThresholdsIVCI.components.a = VK_COMPONENT_SWIZZLE_ONE;
	lightingThresholdsIVCI.format = VK_FORMAT_R8_UNORM;
	lightingThresholdsIVCI.image = lightingThresholds;
	lightingThresholdsIVCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	lightingThresholdsIVCI.subresourceRange.baseArrayLayer = 0;
	lightingThresholdsIVCI.subresourceRange.baseMipLevel = 0;
	lightingThresholdsIVCI.subresourceRange.layerCount = 1;
	lightingThresholdsIVCI.subresourceRange.levelCount = 1;
	lightingThresholdsIVCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	lightingThresholdsIVCI.viewType = VK_IMAGE_VIEW_TYPE_2D;

	if (vkCreateImageView(logicalDevice, &lightingThresholdsIVCI, nullptr, &lightingThresholdsIV) != VK_SUCCESS) {
		throw std::runtime_error("could not create lighting thresholds image view");
	}
		
	//create pool
	VkDescriptorPoolSize descriptorPoolSize{};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolSize samplerPoolSize{};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = static_cast<uint32_t>(50 * MAX_FRAMES_IN_FLIGHT);// i no longer know how many, BUT it depends on how many frames im drawing static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolSize meshModelmatricesPoolSize{};
	meshModelmatricesPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	meshModelmatricesPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolSize lightingThresholdsPoolSize{};
	lightingThresholdsPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	lightingThresholdsPoolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

	VkDescriptorPoolSize allDescriptorPoolSizes[] = { descriptorPoolSize, samplerPoolSize, meshModelmatricesPoolSize, lightingThresholdsPoolSize };
	VkDescriptorPoolCreateInfo descriptorPoolCI{}; //No haces la pool lo suficientemente grande es uno de los problemas que las validation layers de vulkan 1.1 no atrapan, regresan POOL OUT OF MEMORY
	descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCI.pPoolSizes = allDescriptorPoolSizes;
	descriptorPoolCI.poolSizeCount = 4;
	descriptorPoolCI.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);//i can have 2 frames in flight, so i need two sets, each has 1 descriptor, so the size of each Pool is also 2

	if (vkCreateDescriptorPool(logicalDevice, &descriptorPoolCI, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("could not create descriptor pool");
	}

	//alojamos los sets
	VkDescriptorSetAllocateInfo descriptorSetAI{};

	descriptorSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAI.descriptorPool = descriptorPool;
	descriptorSetAI.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, graphicsPipeline->descriptorSetLayout);//vector de longitud 2 con todos los valores inicializados con descriptorSetLayout
	descriptorSetAI.pSetLayouts = descriptorSetLayouts.data();

	if (vkAllocateDescriptorSets(logicalDevice, &descriptorSetAI, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("could not allocate descriptor sets");
	}

	//le metemos los datos estáticos
	for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){

		std::vector<VkWriteDescriptorSet> currWriteDescriptorSets(0);

		//los datos de mas matrices originales
		VkDescriptorBufferInfo currBI{};
		currBI.buffer = uniformBuffers[i];
		currBI.offset = 0;
		currBI.range = sizeof(MatrixTransformations);

		VkWriteDescriptorSet cameraMatricesWrite{};//No funcionaba con arrays de c???
		cameraMatricesWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		cameraMatricesWrite.dstSet = descriptorSets[i];
		cameraMatricesWrite.dstBinding = 0;//el del shader
		cameraMatricesWrite.dstArrayElement = 0;//los descriptores pueden ser arrays, este es el primer indice en el array que queremos actualizar
		cameraMatricesWrite.descriptorCount = 1;//enmpezamos en 0 (dstArrayElement) y actualizamos 1
		cameraMatricesWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		cameraMatricesWrite.pBufferInfo = &currBI;//para buffers
		cameraMatricesWrite.pImageInfo = nullptr;//para image data
		cameraMatricesWrite.pTexelBufferView = nullptr;//para bufferviews

		currWriteDescriptorSets.push_back(cameraMatricesWrite);

		//los datos de las matrices de las meshes
		VkDescriptorBufferInfo currMeshModelMatricesBI{};
		currMeshModelMatricesBI.buffer = meshModelMatricesBuffers[i];//el buffer del frame i
		currMeshModelMatricesBI.offset = 0;
		currMeshModelMatricesBI.range = 50 * 64;//64 es el tamaño de una mat4, 50 es arbitrario

		VkWriteDescriptorSet meshModelMatricesWrite{};
		meshModelMatricesWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		meshModelMatricesWrite.dstSet = descriptorSets[i];
		meshModelMatricesWrite.dstBinding = 2;//el del shader
		meshModelMatricesWrite.dstArrayElement = 0;
		meshModelMatricesWrite.descriptorCount = 1;
		meshModelMatricesWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		meshModelMatricesWrite.pBufferInfo = &currMeshModelMatricesBI;//para buffers
		meshModelMatricesWrite.pImageInfo = nullptr;
		meshModelMatricesWrite.pTexelBufferView = nullptr;

		currWriteDescriptorSets.push_back(meshModelMatricesWrite);

		//los datos de lighting thresholds
		VkDescriptorImageInfo lightingThresholdsII{};
		lightingThresholdsII.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lightingThresholdsII.imageView = lightingThresholdsIV;
		lightingThresholdsII.sampler = lightingThresholdsSampler;

		VkWriteDescriptorSet lightingThresholdsWrite{};
		lightingThresholdsWrite.descriptorCount = 1;
		lightingThresholdsWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		lightingThresholdsWrite.dstArrayElement = 0;
		lightingThresholdsWrite.dstBinding = 3;
		lightingThresholdsWrite.dstSet = descriptorSets[i];
		lightingThresholdsWrite.pImageInfo = &lightingThresholdsII;
		lightingThresholdsWrite.pBufferInfo = nullptr;
		lightingThresholdsWrite.pTexelBufferView = nullptr;
		lightingThresholdsWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		currWriteDescriptorSets.push_back(lightingThresholdsWrite);

		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(currWriteDescriptorSets.size()), currWriteDescriptorSets.data(), 0, nullptr);// lo de copy descriptors es ps para copiarlos
	}

}
void destroyDescriptorSetResources() {
	vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(logicalDevice, uniformBuffers[i], nullptr);
		vkDestroyBuffer(logicalDevice, meshModelMatricesBuffers[i], nullptr);
	}

	vkUnmapMemory(logicalDevice, matrixUniformMemory);//unnecesary, only for explicity
	vkFreeMemory(logicalDevice, matrixUniformMemory, nullptr);

	vkUnmapMemory(logicalDevice, meshModelMatricesMemory);//unnecesary, only for explicity
	vkFreeMemory(logicalDevice, meshModelMatricesMemory, nullptr);

	//lighting thresholds stuff
	vkDestroyImage(logicalDevice, lightingThresholds, nullptr);
	vkDestroySampler(logicalDevice, lightingThresholdsSampler, nullptr);
	vkDestroyImageView(logicalDevice, lightingThresholdsIV, nullptr);

	//free constantUniformsMemory
	vkFreeMemory(logicalDevice, constantUniformsMemory, nullptr);
}
////////////////*classes*/
void dsImageUpdater1::addAlbedoMap(VkImageView albedoMapImageView) {
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorImageInfo currII{};//tengo el sampler y la textura en el descriptor set de los uniforms, ndmas en otro binding
		currII.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;//en este lo tengo en createTextureImage
		currII.imageView = albedoMapImageView;
		currII.sampler = this->albedoSampler;

		VkWriteDescriptorSet currWriteDescriptorSets{};//No funcionaba con arrays de c???
		currWriteDescriptorSets.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		currWriteDescriptorSets.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		currWriteDescriptorSets.dstArrayElement = amountOfLoadedAlbedos;
		currWriteDescriptorSets.descriptorCount = 1;//no need for arbitrary size because i only update one at a time
		currWriteDescriptorSets.dstSet = descriptorSets[i];

		currWriteDescriptorSets.dstBinding = 1;

		currWriteDescriptorSets.pImageInfo = &currII;//no es necesario poner los otros en nullptr?
		currWriteDescriptorSets.pBufferInfo = nullptr;
		currWriteDescriptorSets.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(logicalDevice, 1, &currWriteDescriptorSets, 0, nullptr);// lo de copy descriptors es ps para copiarlos
	}

	amountOfLoadedAlbedos++;
}

dsImageUpdater1::dsImageUpdater1() {
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
	
	//necesitamos saber cuanto de anisotropy soporta el physical device
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
	if (vkCreateSampler(logicalDevice, &testSamplerCI, nullptr, & this->albedoSampler) != VK_SUCCESS) {
		throw std::runtime_error("could not create test sampler");
	}
}

void dsImageUpdater1::del() {
	vkDestroySampler(logicalDevice, this->albedoSampler, nullptr);
}

/*about uniforms*/
/*
-A descriptor says where to read data for uniforms
-for optimization, you send them in sets (descriptorSets)
-theese sets have their memory managed by a pool (descriptorPool)
	-this descriptor pool manages the memory of descriptorSets, not descriptors, so you need to specify the vkDescriptorPoolSize of each type
-you allocate them with alocateInfo
-but they have no data in them, they were only allocated, so you use writeDescriptorSets to actually initialize them
	-these writeDescriptorSets have data for each descriptorSet, so you need to give writeDescriptorSets an array of the different data for each descriptor in the set
		this done with descriptorBufferInfo,descriptorImageInfo,etc
*/