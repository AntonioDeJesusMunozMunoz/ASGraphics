#include <ASG_PerpetualVertexIndexBuffer.hpp>

asgPerpetualVIB::asgPerpetualVIB(uint32_t vertexBufferSize, uint32_t indexBufferSize){
	if (validationLayersEnabled) {
		printf("\n[creating perpetual vertex index buffer]");
	}
	//creo vertexBuffer
	VkBufferCreateInfo VertexBufferCI{};

	queueFamilyIndices queueIndices = getSelectedQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { queueIndices.transferFamilyIndex, queueIndices.graphicsFamilyIndex };
	VertexBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	VertexBufferCI.pQueueFamilyIndices = queueFamilyIndices;
	VertexBufferCI.queueFamilyIndexCount = 2;
	VertexBufferCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
	VertexBufferCI.size = vertexBufferSize;
	VertexBufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;


	if (vkCreateBuffer(logicalDevice, &VertexBufferCI, nullptr, &this->vertexHandle) != VK_SUCCESS) {
		throw std::runtime_error("could not create new vertex buffer");
	}

	// consigo sus mem requirements
	VkMemoryRequirements vertexMemRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, this->vertexHandle, &vertexMemRequirements);

	// creo indexBuffer 
	VkBufferCreateInfo indexBufferCI{};
	indexBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	indexBufferCI.pQueueFamilyIndices = queueFamilyIndices;
	indexBufferCI.queueFamilyIndexCount = 2;
	indexBufferCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
	indexBufferCI.size = indexBufferSize;
	indexBufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	if (vkCreateBuffer(logicalDevice, &indexBufferCI, nullptr, &this->indexHandle) != VK_SUCCESS) {
		throw std::runtime_error("could not create new index buffer");
	}

	// consigo sus mem requirements
	VkMemoryRequirements indexMemRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, this->indexHandle, &indexMemRequirements);

	// alojo memoria 
	VkMemoryAllocateInfo deviceMemoryAI{};

	deviceMemoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	deviceMemoryAI.memoryTypeIndex = findRigthMemoryType(vertexMemRequirements.memoryTypeBits & indexMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);//busco una memoria que sea buena para vertexBuffer y(&) 
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
	this->verticesInside = 0;

	this->indexByteOffset = static_cast<unsigned int>(indexMemRequirements.alignment * ceil(vertexMemRequirements.size / indexMemRequirements.alignment));
	this->indexByteSize = indexBufferSize;
	this->indexBytesUsed = 0;
	this->indicesInside = 0;

	//mapeo los buffers
	vkMapMemory(logicalDevice, this->memory, 0, vertexMemRequirements.size + indexMemRequirements.size, 0, &this->mappedVertexBufferStart);
	this->mappedIndexBufferStart = static_cast<void*>(static_cast<unsigned char*>(this->mappedVertexBufferStart) + this->indexByteOffset);
}

void asgPerpetualVIB::append(std::vector<Vertex> vertexData, std::vector<uint32_t> indexData){
	if (validationLayersEnabled) {
		printf("\n[appending to perpetual vertex index buffer]");
	}
	uint32_t appendedVertexDataSize = static_cast<uint32_t>(vertexData.size() * sizeof(vertexData[0]));
	uint32_t appendedIndexDataSize = static_cast<uint32_t>(indexData.size() * sizeof(indexData[0]));

	bool vertexFits = appendedVertexDataSize < (this->vertexByteSize - this->vertexBytesUsed);
	bool indexFits = appendedIndexDataSize < (this->indexByteSize - this->indexBytesUsed);

	uint32_t allVertexDataSize = this->vertexBytesUsed + appendedVertexDataSize;
	uint32_t allIndexDataSize = this->indexBytesUsed + appendedIndexDataSize;

	if (validationLayersEnabled) {
		printf("\n[right before resize]");
	}

	if (!(vertexFits && indexFits)) {
		this->resize(allVertexDataSize, allIndexDataSize);
	}

	if (validationLayersEnabled) {
		printf("\n[after resize]");
	}

	//appending
	memcpy(static_cast<void*>(static_cast<unsigned char*>(this->mappedVertexBufferStart) + this->vertexBytesUsed), vertexData.data(), appendedVertexDataSize);
	memcpy(static_cast<void*>(static_cast<unsigned char*>(this->mappedIndexBufferStart) + this->indexBytesUsed), indexData.data(), appendedIndexDataSize);

	if (validationLayersEnabled) {
		printf("\n[after memcpy]");
	}

	//udate internal vars
	this->vertexBytesUsed += appendedVertexDataSize;
	this->indexBytesUsed += appendedIndexDataSize;

	this->verticesInside = this->vertexBytesUsed / sizeof(Vertex);
	this->indicesInside = this->indexBytesUsed / sizeof(uint32_t);

	if (validationLayersEnabled) {
		printf("\n[end of append]");
	}
}

void asgPerpetualVIB::del(){
	vkUnmapMemory(logicalDevice, this->memory);
	vkDestroyBuffer(logicalDevice, this->vertexHandle, nullptr);
	vkDestroyBuffer(logicalDevice, this->indexHandle, nullptr);
	vkFreeMemory(logicalDevice, this->memory, nullptr);
}

void asgPerpetualVIB::resize(VkDeviceSize vertexSizeRequired, VkDeviceSize indexSizeRequired) {
	// creo vertexBuffer de tamaño size required
	VkBuffer newVertexBuffer;
	VkBufferCreateInfo newVertexBufferCI{};

	queueFamilyIndices queueIndices = getSelectedQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { queueIndices.transferFamilyIndex, queueIndices.graphicsFamilyIndex };
	newVertexBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	newVertexBufferCI.pQueueFamilyIndices = queueFamilyIndices;
	newVertexBufferCI.queueFamilyIndexCount = 2;
	newVertexBufferCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
	newVertexBufferCI.size = vertexSizeRequired;
	newVertexBufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VkResult createBufferResult = vkCreateBuffer(logicalDevice, &newVertexBufferCI, nullptr, &newVertexBuffer);
	//printf("error: out of host memory = %u, out of device memory = %u, invalid opaque capture:%u ", createBufferResult == VK_ERROR_OUT_OF_HOST_MEMORY, createBufferResult == VK_ERROR_OUT_OF_DEVICE_MEMORY, createBufferResult == VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR);
	if (createBufferResult != VK_SUCCESS) {
		throw std::runtime_error("could not create new vertex buffer");
	}

	// consigo sus mem requirements
	VkMemoryRequirements vertexMemRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, newVertexBuffer, &vertexMemRequirements);

	// creo indexBuffer de tamaño size required
	VkBuffer newIndexBuffer;
	VkBufferCreateInfo newIndexBufferCI{};
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
	VkMemoryAllocateInfo newDeviceMemoryAI{};

	newDeviceMemoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	newDeviceMemoryAI.memoryTypeIndex = findRigthMemoryType(vertexMemRequirements.memoryTypeBits & indexMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);//busco una memoria que sea buena para vertexBuffer y(&) 
	newDeviceMemoryAI.allocationSize = vertexMemRequirements.size + indexMemRequirements.size;

	if (vkAllocateMemory(logicalDevice, &newDeviceMemoryAI, nullptr, &newDeviceMemory) != VK_SUCCESS) {
		throw std::runtime_error("could not allocate vertex buffer memory");
	}

	vkBindBufferMemory(logicalDevice, newVertexBuffer, newDeviceMemory, 0);
	vkBindBufferMemory(logicalDevice, newIndexBuffer, newDeviceMemory, static_cast<unsigned int>(indexMemRequirements.alignment * ceil(vertexMemRequirements.size / indexMemRequirements.alignment)));

	//paso los contenidos
	copyBuffer(this->vertexHandle, 0, newVertexBuffer, 0, this->vertexByteSize);//con VK_WHOLE_SIZE daba validation error diciendo que era un numero muy grande
	copyBuffer(this->indexHandle, 0, newIndexBuffer, 0, std::min(this->indexByteSize, indexSizeRequired));//puede que haya alojado mas index bytes inicialmente de los que necesitaba, si vertex necesita resize pero index no, creara un buffer mas pequeño y podria dar error

	//destruyo lo anterior
	this->del();

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
