#include <ASG_utils.hpp>

//extern variables
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice logicalDevice{};

VkSurfaceKHR windowSurface;//lo usa una función de la swapchain y getSelectedQueueFamilies(osea main), ademas de momento la crea en asInit y existe la posiblidad de que la necesite yo al usarlo(o no)
GLFWwindow* ventana;

VkCommandPool commandPool, transferCommandPool;//las usa main y las definiciones en utils
VkQueue graphicsQueueHandle, presentQueueHandle, transferQueueHandle;

//builtins
#include <vector>

queueFamilyIndices getSelectedQueueFamilies(VkPhysicalDevice device) {
	queueFamilyIndices selectedQueueFamilies{};

	//conseguimos queueFamilyProperties de la misma manera
	uint32_t queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queueFamiliesProperties.data());

	//checamos bitwise si tiene queue de gráficos
	for (int i = 0; i < queueCount; i++) {
		if (queueFamiliesProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {//graphics and compute families implicitly have transfer capabilities, i wanted a challenge so i used a third diferent queue
			selectedQueueFamilies.graphicsFamilyIndex = i;
			selectedQueueFamilies.familiesPresentBitMask |= GRAPHICS_FAMILY_PRESENT;
		}
		else if (queueFamiliesProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
			selectedQueueFamilies.transferFamilyIndex = i;
			selectedQueueFamilies.familiesPresentBitMask |= TRANSFER_FAMILY_PRESENT;
			continue;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, windowSurface, &presentSupport);
		if (presentSupport) {
			selectedQueueFamilies.presentFamilyIndex = i;
			selectedQueueFamilies.familiesPresentBitMask |= PRESENT_FAMILY_PRESENT;
		}

		if (selectedQueueFamilies.hasAllRequirements()) {
			break;
		}
	}

	return selectedQueueFamilies;
}

uint32_t findRigthMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryProperties) {
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

	for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
		if (((1 << i) & typeFilter)  // 1 << i: el bit en la posición i le ponemos valor 1 y checamos si es el que buscamos, literal es solo bitwise curr == wanted
			&& (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties) {//checo si memoryProperties y currPhysicalMemoryProperties tienen las mismas flags
			return i;
		}
	}

	throw std::runtime_error("could not find suitable memory type");
}

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkImageMemoryBarrier barrier{};
	barrier.oldLayout = oldLayout;//este podría ser VK_IMAGE_LAYOUT_UNDEFINED si no te importa el contenido existente en la imagen
	barrier.newLayout = newLayout;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;//ps no estamos cambiando la ownership de la queue family
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;// solo tengo uno de estos
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	VkPipelineStageFlags srcStage, dstStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {//estos atributos de la barrera dependen de la transición
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;//Detendremos el write
		barrier.srcAccessMask = 0;//hasta que nada (osea no lo detenemos)

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;//Entre el inicio y transfer(ponemos el inicio pq ps no esperamos nada)
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;//Detendremos el read
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;//hasta que tranfer termine de escribir

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;//Entre transfer y fragment
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;//detengo las operaciones con 
		barrier.srcAccessMask = 0;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;//Entre inicio y los primeros tests de fragmentos
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else {
		throw std::runtime_error("called layoutTransition with a layout pair that isnt supported by the if");
	}
	//Transfer no es una stage real, es simplemente una "stage" en donde transfers ocurren

	VkCommandBuffer commandBuffer = createSingleUseCmdBuffer();

	vkCmdPipelineBarrier(commandBuffer,
		srcStage,//En que pipeline stage ocurren las operaciones que deben ocurrir antes de la barrera
		dstStage, //En que pipeline stage ocurren las operaciones que deben esperar a la barrera
		0,//la única otra opción es VK_DEPENDENCY_BY_REGION_BIT que permite leer de las regiones donde ya se haya escrito
		0, nullptr, 0, nullptr, 1, &barrier);


	endSingleUseCmdBuffer(commandBuffer, graphicsQueueHandle);
}

VkCommandBuffer createSingleUseCmdBuffer() {//Aloja e inicia un command buffer con ONE_TIME_SUBMIT flag
	VkCommandBufferAllocateInfo cmdBufferAI{};
	cmdBufferAI.commandBufferCount = 1;
	cmdBufferAI.commandPool = commandPool;
	cmdBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

	VkCommandBuffer returnCmdBuffer;
	vkAllocateCommandBuffers(logicalDevice, &cmdBufferAI, &returnCmdBuffer);

	VkCommandBufferBeginInfo cmdBufferBI{};
	cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(returnCmdBuffer, &cmdBufferBI);
	return returnCmdBuffer;
}

void endSingleUseCmdBuffer(VkCommandBuffer buffer, VkQueue queueToSubmit) {
	vkEndCommandBuffer(buffer);
	
	VkSubmitInfo SI{};
	SI.commandBufferCount = 1;
	SI.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SI.pCommandBuffers = &buffer;
	vkQueueSubmit(queueToSubmit, 1, &SI, VK_NULL_HANDLE);
	vkQueueWaitIdle(queueToSubmit);

	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &buffer);
}

void createGenericBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags wantedMemoryProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory, VkSharingMode sharingMode, uint32_t queueAmount, uint32_t* queueFamilyIndices) {
	//llenado
	VkBufferCreateInfo bufferCI{};
	bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

	bufferCI.queueFamilyIndexCount = queueAmount;
	bufferCI.pQueueFamilyIndices = queueFamilyIndices;//estos son para especificar que families comparten este buffer

	bufferCI.sharingMode = sharingMode;//lo usará la transfer y graphics queue, all usar diferente queue para staging buffer puede que vea beneficios en velocidad(más queues es similar a más threads, más rápido, pero solo si no hay demasiada espera para sincronizarlas), también SHARING_MODE_CONCURRENT puede ser más lento que SHARING_MODE_EXCLUSIVE, el industry standard es una queue que solo se dedica a transfers host->driver
	bufferCI.size = size;
	bufferCI.usage = usage;

	if (vkCreateBuffer(logicalDevice, &bufferCI, nullptr, buffer) != VK_SUCCESS) {
		throw std::runtime_error("could not createBuffer");
	}

	//alojar memoria
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findRigthMemoryType(memoryRequirements.memoryTypeBits, wantedMemoryProperties);//queremos una memoria que el host pueda ver y sea coherente con el

	if (vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("could not allocate vertex buffer memory");
	}

	//conectar la memoria al vertex buffer
	vkBindBufferMemory(logicalDevice, *buffer, *bufferMemory, 0);//si el offset no fuera 0, debería ser divisible entre memoryRequirements.alignment

}

void copyBuffer(VkBuffer srcBuffer, VkDeviceSize srcOffset, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size) {
	//alojamos comand buffer para la transferencia
	VkCommandBuffer transferCommandBuffer;
	VkCommandBufferAllocateInfo commandBufferAI{};
	commandBufferAI.commandBufferCount = 1;
	commandBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAI.commandPool = transferCommandPool;

	if (vkAllocateCommandBuffers(logicalDevice, &commandBufferAI, &transferCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("could not create transfer command buffer");
	}

	//lo iniciamos
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;//le decimos que solo se usará una vez

	vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

	//copiamos
	VkBufferCopy copyRegion{};
	copyRegion.dstOffset = dstOffset;
	copyRegion.srcOffset = srcOffset;
	copyRegion.size = size;

	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(transferCommandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	vkQueueSubmit(transferQueueHandle, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueueHandle);//esperamos a que se termine de copiar

	//esta linea de abajo causa errores de validation layers
	//vkFreeCommandBuffers(logicalDevice, commandPool, 1, &transferCommandBuffer);//quizá no necesito liberarlo porque lo libera al destruir su pool
}