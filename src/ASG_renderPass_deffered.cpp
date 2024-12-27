#include <ASG_renderPass_deffered.hpp>

//dependencies
#include <STB/stb_image.h>

//local
#include <binFileLoader.hpp>
#include <ASG_vertex.hpp>

namespace defferedPassFunc {
	//framebuffer stuff
	void createFrameBufferAttachments(asgRenderPass* renderPass, asgSwapChain& sc) {//INFO attachment order: normal, albedo
		//create normal map
		VkImageCreateInfo normalMapCI{};
		normalMapCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		normalMapCI.arrayLayers = 1;
		normalMapCI.extent = { sc.swapExtent.width, sc.swapExtent.height, 1 };
		normalMapCI.format = VK_FORMAT_R8G8B8A8_SRGB;
		normalMapCI.imageType = VK_IMAGE_TYPE_2D;
		normalMapCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		normalMapCI.mipLevels = 1;
		normalMapCI.pQueueFamilyIndices = nullptr;
		normalMapCI.queueFamilyIndexCount = 0;
		normalMapCI.samples = VK_SAMPLE_COUNT_1_BIT;
		normalMapCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		normalMapCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		normalMapCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;//TODO, tiene que ser storage? // VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
		normalMapCI.flags = 0;

		VkImage normalMap;

		if (vkCreateImage(logicalDevice, &normalMapCI, nullptr, &normalMap) != VK_SUCCESS) {
			throw std::runtime_error("could not create normal map");
		}

		//create albedoMap
		VkImageCreateInfo albedoMapCI{};
		albedoMapCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		albedoMapCI.arrayLayers = 1;
		albedoMapCI.extent = { sc.swapExtent.width, sc.swapExtent.height, 1 };
		albedoMapCI.format = VK_FORMAT_B8G8R8A8_SRGB;//PARA EL ALBEDO DEBE SER B8G8R8A8 porque es mas comun que R8G8B8A8 en swapchains
		albedoMapCI.imageType = VK_IMAGE_TYPE_2D;
		albedoMapCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		albedoMapCI.mipLevels = 1;
		albedoMapCI.pQueueFamilyIndices = nullptr;
		albedoMapCI.queueFamilyIndexCount = 0;
		albedoMapCI.samples = VK_SAMPLE_COUNT_1_BIT;
		albedoMapCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		albedoMapCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		albedoMapCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;//TODO, tiene que ser storage? // VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
		albedoMapCI.flags = 0;

		VkImage albedoMap;

		if (vkCreateImage(logicalDevice, &albedoMapCI, nullptr, &albedoMap) != VK_SUCCESS) {
			throw std::runtime_error("could not create albedo map in the deferred render pass function");
		}

		//make memory//TODO make sophisticated memory allocator
		VkMemoryRequirements normalMapMemReq;
		vkGetImageMemoryRequirements(logicalDevice, normalMap, &normalMapMemReq);
		VkMemoryRequirements albedoMapMemReq;
		vkGetImageMemoryRequirements(logicalDevice, albedoMap, &albedoMapMemReq);
		
		VkDeviceSize normalMapMemoryPlusOffset = static_cast<VkDeviceSize>(ceil((static_cast<double>(normalMapMemReq.size)) / albedoMapMemReq.alignment) * albedoMapMemReq.alignment);

		VkMemoryAllocateInfo attachmentMemoryAI{};
		attachmentMemoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		attachmentMemoryAI.allocationSize = normalMapMemoryPlusOffset + albedoMapMemReq.size;
		attachmentMemoryAI.memoryTypeIndex = findRigthMemoryType(normalMapMemReq.memoryTypeBits | albedoMapMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(logicalDevice, &attachmentMemoryAI, nullptr, &renderPass->attachmentMemory) != VK_SUCCESS) {
			throw std::runtime_error("could not allocate memory for attachments");
		}

		vkBindImageMemory(logicalDevice, normalMap, renderPass->attachmentMemory,  0); //OG
		vkBindImageMemory(logicalDevice, albedoMap, renderPass->attachmentMemory, normalMapMemoryPlusOffset); //OG
		
		//create normal image view
		VkImageViewCreateInfo normalImageViewCI{};
		normalImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		normalImageViewCI.image = normalMap;
		normalImageViewCI.format = VK_FORMAT_R8G8B8A8_SRGB;
		normalImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;

		//estos te permiten mucha customización de los color channels
		normalImageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		normalImageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		normalImageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		normalImageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		normalImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;//indicamos que aspectos podemos leer?
		normalImageViewCI.subresourceRange.baseArrayLayer = 0;// no queremos mip maps ni multiple layers
		normalImageViewCI.subresourceRange.baseMipLevel = 0;
		normalImageViewCI.subresourceRange.layerCount = 1;
		normalImageViewCI.subresourceRange.levelCount = 1;

		VkImageView normalMapImageView;
		if (vkCreateImageView(logicalDevice, &normalImageViewCI, nullptr, &normalMapImageView) != VK_SUCCESS) {
			throw std::runtime_error("could not create normal map Image View");
		}

		//create albedo image view
		VkImageViewCreateInfo albedoImageViewCI{};
		albedoImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		albedoImageViewCI.image = albedoMap;
		albedoImageViewCI.format = VK_FORMAT_B8G8R8A8_SRGB;//PARA EL ALBEDO DEBE SER B8G8R8A8 porque es mas comun que R8G8B8A8 en swapchains
		albedoImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;

		//estos te permiten mucha customización de los color channels
		albedoImageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		albedoImageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		albedoImageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		albedoImageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		albedoImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;//indicamos que aspectos podemos leer?
		albedoImageViewCI.subresourceRange.baseArrayLayer = 0;// no queremos mip maps ni multiple layers
		albedoImageViewCI.subresourceRange.baseMipLevel = 0;
		albedoImageViewCI.subresourceRange.layerCount = 1;
		albedoImageViewCI.subresourceRange.levelCount = 1;

		VkImageView albedoMapImageView;
		if (vkCreateImageView(logicalDevice, &albedoImageViewCI, nullptr, &albedoMapImageView) != VK_SUCCESS) {
			throw std::runtime_error("could not create normal map Image View");
		}

		//append the normal stuff to the vectors
		renderPass->attachmentImages.push_back(albedoMap);
		renderPass->attachmentImageViews.push_back(albedoMapImageView);//TODO DEBUG???

		//append the normal stuff to the vectors
		renderPass->attachmentImages.push_back(normalMap);
		renderPass->attachmentImageViews.push_back(normalMapImageView);

	}
	void createFrameBuffers(asgSwapChain& sc, asgRenderPass* renderPass) {
		renderPass->framebuffers.resize(sc.images.size());
		for (size_t i = 0; i < sc.views.size(); i++) {//albedo, normal, depth//TODO make the depth buffer not a part of sc
			//VkImageView currAttachments[] = { swapChain->views[i], swapChain->depthBufferImageView };// framebuffer attachment order : output, normal, depth, albedo
			
			std::vector<VkImageView> currAttachments{ sc.views[i], renderPass->attachmentImageViews[1], sc.depthBufferImageView, renderPass->attachmentImageViews[0] };//the secind attachment is the one that changes wether it runs or not
			
			VkFramebufferCreateInfo currCI{};
			currCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			currCI.renderPass = renderPass->handle; //decimos que debe ser compatible con esta render pass
			currCI.attachmentCount = static_cast<uint32_t>(currAttachments.size());
			currCI.pAttachments = currAttachments.data();//se sabe cual es el depth pq lo especificaste en el attachmentReference
			currCI.width = sc.swapExtent.width;
			currCI.height = sc.swapExtent.height;
			currCI.layers = 1;

			printf("\nrecreating framebuffers for defered pass\n");

			if (vkCreateFramebuffer(logicalDevice, &currCI, nullptr, &(renderPass->framebuffers[i])) != VK_SUCCESS) {
				throw std::runtime_error("could not create frame buffer");
			}
		}
	}

	//gBuffer subpass
	namespace gBufferSubPass {
		//descriptors
		VkDescriptorPool descriptorPool;

		//uniforms
		std::vector<VkBuffer>uniformBuffers(MAX_FRAMES_IN_FLIGHT);
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

		//helpers
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

		//func
		void createPipeline(asgRenderSubPass* gBufferSubPass, VkRenderPass renderPass, uint32_t subPassIndex) {
			//create layout bindings
			VkDescriptorSetLayoutBinding viewProjBufferLB = asgPipelineFunc::createLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT);
			VkDescriptorSetLayoutBinding albedoSamplersLB = asgPipelineFunc::createLayoutBinding(1, 50, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT);
			VkDescriptorSetLayoutBinding meshModelMatricesLB = asgPipelineFunc::createLayoutBinding(2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT);
			VkDescriptorSetLayoutBinding lightingThresholdsLB = asgPipelineFunc::createLayoutBinding(3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT);

			//create descriptorSetLayout for this pipeline
			std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { viewProjBufferLB, albedoSamplersLB, meshModelMatricesLB, lightingThresholdsLB };
			gBufferSubPass->pipeline.descriptorSetLayout = asgPipelineFunc::createDescriptorSetLayout(layoutBindings);

			//create shader modules
			VkShaderModule vertexShaderModule = asgPipelineFunc::createShaderModule(readRawBinary("./resourceFiles/shaderPrograms/compiled/gBufferPass.vert.spv"));
			VkShaderModule fragmentShaderModule = asgPipelineFunc::createShaderModule(readRawBinary("./resourceFiles/shaderPrograms/compiled/gBufferPass.frag.spv"));

			//fill shader stages cis
			VkPipelineShaderStageCreateInfo vertexShaderStageCI = asgPipelineFunc::fillShaderStageCI(VK_SHADER_STAGE_VERTEX_BIT, "main", vertexShaderModule);
			VkPipelineShaderStageCreateInfo fragmentShaderStageCI = asgPipelineFunc::fillShaderStageCI(VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragmentShaderModule);
			VkPipelineShaderStageCreateInfo shaderStagesCreateInfo[] = { vertexShaderStageCI, fragmentShaderStageCI };

			//fill dynamic state ci
			std::vector<VkDynamicState> dynamicState = { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };
			VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = asgPipelineFunc::fillDynamicStateCI(&dynamicState);

			//Attribute descriptions

			std::vector<VkVertexInputAttributeDescription>defaultVertexInputAttrDesc = asgPipelineFunc::fillDefaultVertexInputAttrDescs();
			//binding descriptors
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;//indice de este binding en el array de bindings
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;//cada chunk corresponde a un vertice
			bindingDescription.stride = sizeof(Vertex);

			//Vertex input state, decimos en que formato le damos los vertices
			VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
			vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
			vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(defaultVertexInputAttrDesc.size());
			vertexInputStateCreateInfo.pVertexAttributeDescriptions = defaultVertexInputAttrDesc.data();

			//depth stencil state
			VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
			depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencilStateCI.depthTestEnable = VK_TRUE;
			depthStencilStateCI.depthWriteEnable = VK_TRUE;
			depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;//solo quedarse con los fragmentos que estan entre maxDetphBounds y minDepthBounds
			depthStencilStateCI.maxDepthBounds = 1.0f;
			depthStencilStateCI.minDepthBounds = 0.0f;

			depthStencilStateCI.stencilTestEnable = VK_FALSE;
			depthStencilStateCI.back = {};//estos son del stencil
			depthStencilStateCI.front = {};

			//Input assembly, que va a dibujar con este input y como
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
			inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
			inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
			viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCreateInfo.scissorCount = 1;
			viewportStateCreateInfo.viewportCount = 1;

			//rasterization
			VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = asgPipelineFunc::fillDefaultRasterizationCI();

			//multisampling
			VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = asgPipelineFunc::fillNoMultisampleCI();

			//Color blending, necesite global y perframebuffer (que significa esto?)
			VkPipelineColorBlendAttachmentState blendAttachmentState{};//para añadir transparencia, solo adivinas los enums basado en como quieres que los combine
			blendAttachmentState.blendEnable = VK_FALSE;
			blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
			blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
			blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;

			std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates{ blendAttachmentState, blendAttachmentState };//it would appear these are per color attachment
			VkPipelineColorBlendStateCreateInfo blendCreateInfo = asgPipelineFunc::fillColorBlendCI(&blendAttachmentStates, { 0.0f,0.0f,0.0f,0.0f });

			/*push constant range*/
			VkPushConstantRange pcRange{};
			pcRange.offset = 0;
			pcRange.size = sizeof(pushConstants) + 4;//TODO 4
			pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			std::vector<VkPushConstantRange> pcRanges = { pcRange };//puedes hacer mas de 1 range, pero la especificacion dice que solo uno por shader stage

			/*pipeline layout*///maneja las uniform
			VkPipelineLayoutCreateInfo pipelineLayoutCI{};
			pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCI.setLayoutCount = 1;
			pipelineLayoutCI.pSetLayouts = &gBufferSubPass->pipeline.descriptorSetLayout;
			pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(pcRanges.size());
			pipelineLayoutCI.pPushConstantRanges = pcRanges.data();

			if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCI, nullptr, &gBufferSubPass->pipeline.pipelineLayout) != VK_SUCCESS) {
				throw std::runtime_error("could not create pipeline layout");
			}

			//ya crear la pipeline
			VkGraphicsPipelineCreateInfo graphicsPipelineCI{};
			graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;//opcional, es para crear subpipelines que es más rapido que varias pipelines
			graphicsPipelineCI.basePipelineIndex = -1;//opcional, tiene que ver con parametro anterior
			graphicsPipelineCI.layout = gBufferSubPass->pipeline.pipelineLayout;
			graphicsPipelineCI.renderPass = renderPass;
			graphicsPipelineCI.stageCount = 2;
			graphicsPipelineCI.pStages = shaderStagesCreateInfo;
			graphicsPipelineCI.pVertexInputState = &vertexInputStateCreateInfo;
			graphicsPipelineCI.pInputAssemblyState = &inputAssemblyCreateInfo;
			graphicsPipelineCI.pViewportState = &viewportStateCreateInfo;
			graphicsPipelineCI.pColorBlendState = &blendCreateInfo;
			graphicsPipelineCI.pDynamicState = &dynamicStateCreateInfo;
			graphicsPipelineCI.pDepthStencilState = &depthStencilStateCI;
			graphicsPipelineCI.pMultisampleState = &multisampleCreateInfo;
			graphicsPipelineCI.pRasterizationState = &rasterizationCreateInfo;
			graphicsPipelineCI.subpass = subPassIndex;

			if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, &gBufferSubPass->pipeline.handle) != VK_SUCCESS) {
				throw std::runtime_error("could not create graphics pipeline");
			}

			//Destroy shader modules as soon as the code is in te pipeline just like openGL
			vkDestroyShaderModule(logicalDevice, vertexShaderModule, nullptr);
			vkDestroyShaderModule(logicalDevice, fragmentShaderModule, nullptr);
		}
		void initializeDescriptorSets(asgRenderSubPass *gBufferSubPass) {
			//createUniformBuffers
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
			if (lightingThresholdsData == nullptr) {
				std::cerr << stbi_failure_reason() << std::endl;
				throw std::runtime_error("could not load image");
			}

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
			//TODO i could make initializeDescriptorSets create all the different layouts i could use and the just pick and chose them when creating the pipelinesa
			std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, gBufferSubPass->pipeline.descriptorSetLayout);//vector de longitud 2 con todos los valores inicializados con descriptorSetLayout
			descriptorSetAI.pSetLayouts = descriptorSetLayouts.data();
			
			gBufferSubPass->descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
			if (vkAllocateDescriptorSets(logicalDevice, &descriptorSetAI, gBufferSubPass->descriptorSets.data()) != VK_SUCCESS) {
				throw std::runtime_error("could not allocate descriptor sets");
			}

			//le metemos los datos estáticos
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

				std::vector<VkWriteDescriptorSet> currWriteDescriptorSets(0);

				//los datos de mas matrices originales
				VkDescriptorBufferInfo currBI{};
				currBI.buffer = uniformBuffers[i];
				currBI.offset = 0;
				currBI.range = sizeof(MatrixTransformations);

				VkWriteDescriptorSet cameraMatricesWrite{};//No funcionaba con arrays de c???
				cameraMatricesWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				cameraMatricesWrite.dstSet = gBufferSubPass->descriptorSets[i];
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
				meshModelMatricesWrite.dstSet = gBufferSubPass->descriptorSets[i];
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
				lightingThresholdsWrite.dstSet = gBufferSubPass->descriptorSets[i];
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
	}

	//ligthing subpass
	namespace ligthingSubPass {
		VkDescriptorPool descriptorPool;

		void createPipeline(asgRenderSubPass* ligthingSubPass, VkRenderPass renderPass, uint32_t subPassIndex) {
			//create layout bindings
			VkDescriptorSetLayoutBinding albedoInputAttachmentLB = asgPipelineFunc::createLayoutBinding(1, 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
				VK_SHADER_STAGE_FRAGMENT_BIT);
			VkDescriptorSetLayoutBinding normalInputAttachmentLB = asgPipelineFunc::createLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
				VK_SHADER_STAGE_FRAGMENT_BIT);


			//create descriptorSetLayout for this pipeline
			std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { normalInputAttachmentLB, albedoInputAttachmentLB};//TODO POSSIBLE BUG parece que tengo los binding indices diferentes a como estan en la salida, aunque estan en el mismo orden que aparecen cuando creo los framebuffers
			ligthingSubPass->pipeline.descriptorSetLayout = asgPipelineFunc::createDescriptorSetLayout(layoutBindings);

			//create shader modules
			VkShaderModule vertexShaderModule = asgPipelineFunc::createShaderModule(readRawBinary("./resourceFiles/shaderPrograms/compiled/lightingPass.vert.spv"));
			VkShaderModule fragmentShaderModule = asgPipelineFunc::createShaderModule(readRawBinary("./resourceFiles/shaderPrograms/compiled/lightingPass.frag.spv"));

			//fill shader stages cis
			VkPipelineShaderStageCreateInfo vertexShaderStageCI = asgPipelineFunc::fillShaderStageCI(VK_SHADER_STAGE_VERTEX_BIT, "main", vertexShaderModule);
			VkPipelineShaderStageCreateInfo fragmentShaderStageCI = asgPipelineFunc::fillShaderStageCI(VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragmentShaderModule);
			VkPipelineShaderStageCreateInfo shaderStagesCreateInfo[] = { vertexShaderStageCI, fragmentShaderStageCI };

			//fill dynamic state ci
			std::vector<VkDynamicState> dynamicState = { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };
			VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = asgPipelineFunc::fillDynamicStateCI(&dynamicState);

			//Vertex input state
			VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
			vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
			vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
			vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
			vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

			//depth stencil state
			VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
			depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencilStateCI.depthTestEnable = VK_TRUE;
			depthStencilStateCI.depthWriteEnable = VK_FALSE;
			depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;//solo quedarse con los fragmentos que estan entre maxDetphBounds y minDepthBounds
			depthStencilStateCI.maxDepthBounds = 1.0f;
			depthStencilStateCI.minDepthBounds = 0.0f;

			depthStencilStateCI.stencilTestEnable = VK_FALSE;
			depthStencilStateCI.back = {};//estos son del stencil
			depthStencilStateCI.front = {};

			//Input assembly, que va a dibujar con este input y como
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
			inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
			inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

			VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
			viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCreateInfo.scissorCount = 1;
			viewportStateCreateInfo.viewportCount = 1;

			//rasterization
			VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = asgPipelineFunc::fillDefaultRasterizationCI();

			//multisampling
			VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = asgPipelineFunc::fillNoMultisampleCI();

			//Color blending, necesite global y perframebuffer (que significa esto?)
			VkPipelineColorBlendAttachmentState blendAttachmentState{};//para añadir transparencia, solo adivinas los enums basado en como quieres que los combine
			blendAttachmentState.blendEnable = VK_FALSE;
			blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
			blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
			blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;

			std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates{ blendAttachmentState };//it would appear these are per color attachment
			VkPipelineColorBlendStateCreateInfo blendCreateInfo = asgPipelineFunc::fillColorBlendCI(&blendAttachmentStates, { 0.0f,0.0f,0.0f,0.0f });

			//push constant ranges
			std::vector<VkPushConstantRange> pcRanges{};

			/*pipeline layout*///maneja las uniform
			VkPipelineLayoutCreateInfo pipelineLayoutCI{};
			pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCI.setLayoutCount = 1;
			pipelineLayoutCI.pSetLayouts = &ligthingSubPass->pipeline.descriptorSetLayout;
			pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(pcRanges.size());
			pipelineLayoutCI.pPushConstantRanges = pcRanges.data();

			if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCI, nullptr, &ligthingSubPass->pipeline.pipelineLayout) != VK_SUCCESS) {
				throw std::runtime_error("could not create ligthing subPass pipeline layout");
			}

			//ya crear la pipeline
			VkGraphicsPipelineCreateInfo graphicsPipelineCI{};
			graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;
			graphicsPipelineCI.basePipelineIndex = -1;
			graphicsPipelineCI.layout = ligthingSubPass->pipeline.pipelineLayout;
			graphicsPipelineCI.renderPass = renderPass;
			graphicsPipelineCI.stageCount = 2;
			graphicsPipelineCI.pStages = shaderStagesCreateInfo;
			graphicsPipelineCI.pVertexInputState = &vertexInputStateCreateInfo;
			graphicsPipelineCI.pInputAssemblyState = &inputAssemblyCreateInfo;
			graphicsPipelineCI.pViewportState = &viewportStateCreateInfo;
			graphicsPipelineCI.pColorBlendState = &blendCreateInfo;
			graphicsPipelineCI.pDynamicState = &dynamicStateCreateInfo;
			graphicsPipelineCI.pDepthStencilState = &depthStencilStateCI;
			graphicsPipelineCI.pMultisampleState = &multisampleCreateInfo;
			graphicsPipelineCI.pRasterizationState = &rasterizationCreateInfo;
			graphicsPipelineCI.subpass = subPassIndex;

			if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, &ligthingSubPass->pipeline.handle) != VK_SUCCESS) {
				throw std::runtime_error("could not create graphics pipeline");
			}

			//Destroy shader modules as soon as the code is in te pipeline just like openGL
			vkDestroyShaderModule(logicalDevice, vertexShaderModule, nullptr);
			vkDestroyShaderModule(logicalDevice, fragmentShaderModule, nullptr);
		}
		void initializeDescriptorSets(asgRenderPass& renderPass, asgSwapChain& sc, asgRenderSubPass *ligthingSubPass) {
			//llenar pool sizes
			VkDescriptorPoolSize albedoInputAttachmentPoolSize{};
			albedoInputAttachmentPoolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
			albedoInputAttachmentPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

			VkDescriptorPoolSize normalInputAttachmentPoolSize{};
			normalInputAttachmentPoolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
			normalInputAttachmentPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

			//crear pool
			std::vector<VkDescriptorPoolSize> poolSizes{albedoInputAttachmentPoolSize ,normalInputAttachmentPoolSize };

			VkDescriptorPoolCreateInfo poolCI{};
			poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolCI.maxSets = MAX_FRAMES_IN_FLIGHT;
			poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolCI.pPoolSizes = poolSizes.data();

			if (vkCreateDescriptorPool(logicalDevice, &poolCI, nullptr, &descriptorPool) != VK_SUCCESS) {
				throw std::runtime_error("could not create ligthing pass descriptor pool");
			}

			//alojar descriptor sets
			VkDescriptorSetAllocateInfo descriptorSetAI{};
			descriptorSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAI.descriptorPool = descriptorPool;
			descriptorSetAI.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
			std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, ligthingSubPass->pipeline.descriptorSetLayout);//vector de longitud 2 con todos los valores inicializados con descriptorSetLayout
			descriptorSetAI.pSetLayouts = descriptorSetLayouts.data();

			ligthingSubPass->descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

			if (vkAllocateDescriptorSets(logicalDevice, &descriptorSetAI, ligthingSubPass->descriptorSets.data()) != VK_SUCCESS) {
				throw std::runtime_error("could not allocate ligthing pass descriptor sets");
			}

			//meterle las image view de los input attachment
			for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				std::vector<VkWriteDescriptorSet> writes(0);

				//add all of the attachment image views	
				std::vector<VkDescriptorImageInfo> IIs(renderPass.attachmentImageViews.size());//in the end, the error were the friends we made along the way
				for (int j = 0; j < renderPass.attachmentImageViews.size(); j++) {				
					//llenar currII
					IIs[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					IIs[j].sampler = VK_NULL_HANDLE;
					IIs[j].imageView = renderPass.attachmentImageViews[j];

					VkWriteDescriptorSet currInputWrite{};
					currInputWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					currInputWrite.descriptorCount = 1;
					currInputWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
					currInputWrite.dstArrayElement = 0;
					currInputWrite.dstBinding = j;
					currInputWrite.dstSet = ligthingSubPass->descriptorSets[i];
					currInputWrite.pImageInfo = &IIs[j];//holy fuck, like honestly holy fuck, the error was creating the image info inside the loop and then getting a pointer to it

					//chatgpt explanation
					/*
					The issue lies in how currII is being constructed and assigned. While currII is correctly initialized for each iteration of the loop, the memory address of currII
					is passed to currInputWrite.pImageInfo each time. Since currII is a stack variable that gets reused for every iteration, all VkWriteDescriptorSet entries in writes
					end up pointing to the same (last updated) currII instance.

					When you create a variable inside a loop, it is redeclared and reused in each iteration. 
					Specifically, its memory address remains the same across iterations because it is allocated on the stack.
					*/

					writes.push_back(currInputWrite);
				}

				vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
			}
		}
		void destroyDescriptorSetResources() {
			vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
		}
	}
}

asgRenderPass createDefferedRenderingPass(asgSwapChain& sc) {
	//fill the attachment descriptions
	VkAttachmentDescription albedoAtt{};
	albedoAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	albedoAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	albedoAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	albedoAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	albedoAtt.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;//TODO
	albedoAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	albedoAtt.format = sc.surfaceFormat.format;
	albedoAtt.samples = VK_SAMPLE_COUNT_1_BIT;

	VkAttachmentDescription normalAtt{};
	normalAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	normalAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;//TODO
	normalAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	normalAtt.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	normalAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	normalAtt.format = VK_FORMAT_R8G8B8A8_SRGB;
	normalAtt.samples = VK_SAMPLE_COUNT_1_BIT;

	VkAttachmentDescription depthAtt{};
	depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAtt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAtt.format = sc.depthBufferFormat;
	depthAtt.samples = VK_SAMPLE_COUNT_1_BIT;

	VkAttachmentDescription ligthingOutputAtt{};
	ligthingOutputAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	ligthingOutputAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ligthingOutputAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ligthingOutputAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	ligthingOutputAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;//TODO
	ligthingOutputAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ligthingOutputAtt.format = sc.surfaceFormat.format;
	ligthingOutputAtt.samples = VK_SAMPLE_COUNT_1_BIT;

	//fill g Buffer attachment reference
	VkAttachmentReference gBufferPassAlbedoRef{};
	gBufferPassAlbedoRef.attachment = 3;
	gBufferPassAlbedoRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference gBufferPassNormalRef{};
	gBufferPassNormalRef.attachment = 1;
	gBufferPassNormalRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference gBufferPassDepthRef{};
	gBufferPassDepthRef.attachment = 2;
	gBufferPassDepthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//fill the g Buffer subpass description
	std::vector<VkAttachmentReference> gBufferAttachmentReferences{ gBufferPassAlbedoRef, gBufferPassNormalRef };
	std::vector<VkAttachmentReference> gBufferInputAttachmentReferences{};
	asgRenderSubPass gBufferSubPass;
	gBufferSubPass.fillSubPassDescription(&gBufferAttachmentReferences, &gBufferPassDepthRef, &gBufferInputAttachmentReferences);

	//fill ligthing attachment reference
	VkAttachmentReference ligthingPassOutputRef{};
	ligthingPassOutputRef.attachment = 0;
	ligthingPassOutputRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference ligthingPassNormalRef{};
	ligthingPassNormalRef.attachment = 1;
	ligthingPassNormalRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference ligthingPassDepthRef{};
	ligthingPassDepthRef.attachment = 2;
	ligthingPassDepthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	VkAttachmentReference ligthingPassAlbedoRef{};
	ligthingPassAlbedoRef.attachment = 3;
	ligthingPassAlbedoRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	//fill ligthing subpass description
	std::vector<VkAttachmentReference> ligthingAttachmentReferences{ ligthingPassOutputRef };
	std::vector<VkAttachmentReference> ligthingInputAttachmentReferences{ ligthingPassAlbedoRef, ligthingPassNormalRef };
	asgRenderSubPass ligthingSubPass;
	ligthingSubPass.fillSubPassDescription(&ligthingAttachmentReferences, &ligthingPassDepthRef, &ligthingInputAttachmentReferences);

	//creamos subpass dependencies/*describe dependencias (especifica memory y execution dependencies entre subpasses) para q el driver se encargue de eso (es optimo) //tenemos 3 subpasses??, la que creamos, la operación antes y la operación después, vulkan tiene built-in dependencies que lidian con ellas pero hay que sincronizar la de la operación después??*/
	VkSubpassDependency beginToFirstSubPass{};
	beginToFirstSubPass.srcSubpass = VK_SUBPASS_EXTERNAL; // de quien dependo (indice) VK_SUBPASS_EXTERNAL se refiere a la operación antes o después
	beginToFirstSubPass.dstSubpass = 0;//quien soy yo (indice)
	beginToFirstSubPass.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;//esperaremos a esta operación????
	beginToFirstSubPass.srcAccessMask = 0;
	beginToFirstSubPass.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;//esta operación será la que espere???
	beginToFirstSubPass.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;//específicamente esperaremos hasta que acabe y luego escribiremos???
	
	VkSubpassDependency firstToSecondSubPass{};
	firstToSecondSubPass.srcSubpass = 0;  // First subpass index
	firstToSecondSubPass.dstSubpass = 1;  // Second subpass index
	firstToSecondSubPass.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	firstToSecondSubPass.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	firstToSecondSubPass.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	firstToSecondSubPass.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

	std::vector<VkSubpassDependency> dependencies{ beginToFirstSubPass, firstToSecondSubPass };//beginToFirstSubPass//TODO DEBUG this used to be in the vector but i removed it and nothing happened???

	//fill renderPassCI
	std::vector<VkAttachmentDescription>attachments{ ligthingOutputAtt, normalAtt, depthAtt, albedoAtt };
	std::vector<VkSubpassDescription> subpassDescriptions{ gBufferSubPass.description, ligthingSubPass.description };

	VkRenderPassCreateInfo renderPassCI{};
	renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCI.pAttachments = attachments.data();//cada subpass referencia un attachment????
	renderPassCI.pSubpasses = subpassDescriptions.data();//subpasses, juntar todos los efectos de post procesamiento en una pass usando subpasses le permite al gpu o a vulkan hacer optimizaciones
	renderPassCI.pDependencies = dependencies.data();
	renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassCI.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
	renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size());

	VkRenderPass handle;
	if (vkCreateRenderPass(logicalDevice, &renderPassCI, nullptr, &handle) != VK_SUCCESS) {
		throw std::runtime_error("could not create deffered render pass");
	}

	//creamos asgRenderPass
	asgRenderPass defferedPass{};
	defferedPass.handle = handle;

	//creamos framebuffer attachments
	defferedPassFunc::createFrameBufferAttachments(&defferedPass, sc);

	//creamos framebuffers
	defferedPass._createFrameBuffersFunc = defferedPassFunc::createFrameBuffers;
	defferedPass.createFrameBuffers(sc);

	//creamos graphicsPipelines//TODO, make all of these pipelines blood related so they switch bindings faster
	defferedPassFunc::gBufferSubPass::createPipeline(&gBufferSubPass, defferedPass.handle, 0);
	defferedPassFunc::ligthingSubPass::createPipeline(&ligthingSubPass, defferedPass.handle, 1);

	//initializamos los descriptorSets
	defferedPassFunc::gBufferSubPass::initializeDescriptorSets(&gBufferSubPass);
	defferedPassFunc::ligthingSubPass::initializeDescriptorSets(defferedPass, sc,&ligthingSubPass);

	//metemos las subpasses a el vector
	defferedPass.subPasses.push_back(gBufferSubPass);
	defferedPass.subPasses.push_back(ligthingSubPass);

	return defferedPass;
}
