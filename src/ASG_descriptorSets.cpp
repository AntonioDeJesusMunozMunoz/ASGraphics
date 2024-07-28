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
uint32_t amountOfLoadedAlbedos = 0;//hiba a pensar en como lidiar con unloading de imageViews, pero nada del código lidia con eso así q ps así lo dejo y cuando lidie con unloading lo hago todo junto (pq la mayoria será después de implementar vma

//////////////////*Func*/
void initializeDescriptorSets(asgPipeline *graphicsPipeline) {
	/*createUniformBuffers*/
	std::array<VkMemoryRequirements, MAX_FRAMES_IN_FLIGHT> uniformBuffersMemoryRequirements;
	uint32_t totalMemorySize = 0;
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
		totalMemorySize += static_cast<uint32_t>(uniformBuffersMemoryRequirements[i].size + uniformBuffersMemoryRequirements[i].alignment);//le añado el alignment para asegurarme que la memoria tenga suficiente espacio para alinear los buffers
	}

	//allocate Memory
	VkMemoryAllocateInfo matrixUniformMemoryAI{};
	matrixUniformMemoryAI.allocationSize = totalMemorySize;
	matrixUniformMemoryAI.memoryTypeIndex = findRigthMemoryType(uniformBuffersMemoryRequirements[0].memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);//aunq aquí ya me dió hueva y solo chequé por un buffer
	matrixUniformMemoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	if (vkAllocateMemory(logicalDevice, &matrixUniformMemoryAI, nullptr, &matrixUniformMemory) != VK_SUCCESS) {
		throw std::runtime_error("could not allocate uniform buffer memory");
	}

	/*bind and map buffers*/
	//map the memory
	vkMapMemory(logicalDevice, matrixUniformMemory, 0, totalMemorySize, 0, &mappedUniformBufferMemories[0]);

	uint32_t currMemOffset = 0;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		currMemOffset = static_cast<uint32_t>(uniformBuffersMemoryRequirements[i].alignment * ceil(currMemOffset / uniformBuffersMemoryRequirements[i].alignment));//modify the offset to fit with my alignment

		vkBindBufferMemory(logicalDevice, uniformBuffers[i], matrixUniformMemory, currMemOffset);
		mappedUniformBufferMemories[i] = static_cast<void*>(static_cast<char*>(mappedUniformBufferMemories[0]) + currMemOffset);

		currMemOffset += static_cast<uint32_t>(uniformBuffersMemoryRequirements[i].size);//add my size to the offset for the next buffer
	}
	VkDescriptorPoolSize descriptorPoolSize{};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolSize samplerPoolSize{};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = static_cast<uint32_t>(50 * MAX_FRAMES_IN_FLIGHT);// i no longer know how many, BUT it depends on how many frames im drawing static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolSize allDescriptorPoolSizes[] = { descriptorPoolSize, samplerPoolSize };
	//No haces la pool lo suficientemente grande es uno de los problemas que las validation layers de vulkan 1.1 no atrapan, regresan POOL OUT OF MEMORY
	VkDescriptorPoolCreateInfo descriptorPoolCI{};
	descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCI.pPoolSizes = allDescriptorPoolSizes;
	descriptorPoolCI.poolSizeCount = 2;
	descriptorPoolCI.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);//i can have 2 frames in flight, so i need two sets, each has 1 descriptor, so the size of each Pool is also 2

	if (vkCreateDescriptorPool(logicalDevice, &descriptorPoolCI, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("could not crate descriptor pool");
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
		VkDescriptorBufferInfo currBI{};
		currBI.buffer = uniformBuffers[i];
		currBI.offset = 0;
		currBI.range = sizeof(MatrixTransformations);

		VkWriteDescriptorSet currWriteDescriptorSets{};//No funcionaba con arrays de c???
		currWriteDescriptorSets.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		currWriteDescriptorSets.dstSet = descriptorSets[i];
		currWriteDescriptorSets.dstBinding = 0;//el del shader
		currWriteDescriptorSets.dstArrayElement = 0;//los descriptores pueden ser arrays, este es el primer indice en el array que queremos actualizar
		currWriteDescriptorSets.descriptorCount = 1;//enmpezamos en 0 (dstArrayElement) y actualizamos 1
		currWriteDescriptorSets.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		//solo llenas el tipo de descriptor estes usando
		currWriteDescriptorSets.pBufferInfo = &currBI;//para buffers
		currWriteDescriptorSets.pImageInfo = nullptr;//para image data
		currWriteDescriptorSets.pTexelBufferView = nullptr;//para bufferviews

		vkUpdateDescriptorSets(logicalDevice, 1, &currWriteDescriptorSets, 0, nullptr);// lo de copy descriptors es ps para copiarlos
	}

}
void destroyDescriptorSetResources() {
	vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(logicalDevice, uniformBuffers[i], nullptr);
	}

	vkUnmapMemory(logicalDevice, matrixUniformMemory);//unnecesary, only for explicity
	vkFreeMemory(logicalDevice, matrixUniformMemory, nullptr);
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