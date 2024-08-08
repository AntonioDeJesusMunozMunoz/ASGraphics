#include <ASG_graphicsPipeline.hpp>
//dependencies

//builtin
#include <vector>
//local
#include <dependencies/binFileLoader.hpp>
#include <ASG_vertex.hpp>

/////*funciones*///////
/*declaraciones*/
VkShaderModule createShaderModule(std::vector<unsigned char> rawDataVector);

/*definiciones*/
VkShaderModule createShaderModule(std::vector<unsigned char> rawDataVector) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(rawDataVector.data());
	createInfo.codeSize = rawDataVector.size();

	VkShaderModule returnModule;
	if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &returnModule) != VK_SUCCESS) {
		throw std::runtime_error("\ncould not create shader module\n");
	}
	return returnModule;
}
asgPipeline::asgPipeline(VkFormat chosenSwapSurfaceFormat, VkFormat supportedDepthBufferFormat){
	/*descriptor set layout*/
	//creamos los layout binding
	VkDescriptorSetLayoutBinding matrixBufferLayoutBinding{};
	matrixBufferLayoutBinding.binding = 0;
	matrixBufferLayoutBinding.descriptorCount = 1;//se suele hacer de varios, por ejemplos para dar la matrix de cada transformación de cada parte de un esqueleto
	matrixBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	matrixBufferLayoutBinding.pImmutableSamplers = nullptr;
	matrixBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 50;//TODO este es un número arbitrario de imágenes, si lo cambias, checa que no entre en conflicto con el tamaño de su pool
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding meshModelMatrixLB{};
	meshModelMatrixLB.binding = 2;
	meshModelMatrixLB.descriptorCount = 1;
	meshModelMatrixLB.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	meshModelMatrixLB.pImmutableSamplers = nullptr;
	meshModelMatrixLB.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding lightingThresholdsLB{};//si fuera a hacer otro descriptor set que estuviera siempre conectado, este seria parte de él
	lightingThresholdsLB.binding = 3;
	lightingThresholdsLB.descriptorCount = 1;
	lightingThresholdsLB.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	lightingThresholdsLB.pImmutableSamplers = nullptr;//inmutable samplers is if this descriptor points to some samplers, you can give them here (if they dont need updating ofc), and not worry about them anymore
	lightingThresholdsLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//todos los descriptor sets bindings se combinan en un VkDescriptorSetLayout
	std::vector<VkDescriptorSetLayoutBinding> allLayoutBindings = { matrixBufferLayoutBinding, samplerLayoutBinding, meshModelMatrixLB, lightingThresholdsLB };

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
	descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(allLayoutBindings.size());
	descriptorSetLayoutCI.pBindings = allLayoutBindings.data();
	descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	if (vkCreateDescriptorSetLayout(logicalDevice, &descriptorSetLayoutCI, nullptr, &this->descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("could not create descriptor set layout");
	}

	/*shader stages*/
	//read program data
	std::vector<unsigned char> vertexData = readRawBinary("./resourceFiles/shaderPrograms/compiled/speedShading_0_1.vert.spv");
	std::vector<unsigned char> fragmentData = readRawBinary("./resourceFiles/shaderPrograms/compiled/speedShading_0_1.frag.spv");

	//create shader modules
	VkShaderModule vertexShaderModule = createShaderModule(vertexData);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentData);

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

	VkPipelineShaderStageCreateInfo shaderStagesCreateInfo[] = { vertexStageCreateinfo, fragmentStageCreateInfo };
	
	/*fixed functions*/
	//dynamic state, lo que si se puede cambiar sin recrear toda la pipeline
	std::vector<VkDynamicState> dynamicState = { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicState.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicState.data();

	//Vertex input state, decimos en que formato le damos los vertices
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	//binding descriptors
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;//indice de este binding en el array de bindings
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;//cada chunk corresponde a un vertice
	bindingDescription.stride = sizeof(Vertex);

	//Attribute descriptions
	VkVertexInputAttributeDescription attributeDescriptions[4]{};
	attributeDescriptions[0].binding = 0;//el mismo que su binding descriptor
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].offset = offsetof(Vertex, imgPos);

	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].offset = offsetof(Vertex, normal);

	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 4;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions;

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

	VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo{};
	rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationCreateInfo.depthClampEnable = VK_FALSE;//clampea lo que se sale del frustum en vez de cortarlo
	rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;//la geometria nunca deja el resterizador
	rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationCreateInfo.lineWidth = 1.0f;//medido en fragmentos
	rasterizationCreateInfo.cullMode = VK_CULL_MODE_NONE; //VK_CULL_MODE_BACK_BIT;//
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
	blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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
	
	/*push constant range*/
	VkPushConstantRange pcRange{};
	pcRange.offset = 0;
	pcRange.size = sizeof(pushConstants) + 4;//TODO 4
	pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	
	std::vector<VkPushConstantRange> pcRanges = { pcRange};//puedes hacer mas de 1 range, pero la especificacion dice que solo uno por shader stage

	/*pipeline layout*///maneja las uniform
	VkPipelineLayoutCreateInfo pipelineLayoutCI{};
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts = &this->descriptorSetLayout;

	pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(pcRanges.size());
	pipelineLayoutCI.pPushConstantRanges = pcRanges.data();
	
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCI, nullptr, &this->pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("could not create pipeline");
	}

	/*render passes*/
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = chosenSwapSurfaceFormat;//debe ser el mismo pq ps ese usamos
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
	colorAttachmentReference.attachment = 0;//indice en el array de attachments
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;//el layout con el que trataré ese attachment, en este caso como color buffer

	//las de depth buffer	
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = supportedDepthBufferFormat;//debe ser el mismo pq ps ese usamos
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;//no multisampling
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;//what to do before and after rendering
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;//aqui nos vale vrg a diferencia del color attachment
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference{};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	//cada subpass necesita descripción
	VkSubpassDescription subpassDescription{};//la subpass de color y de depth es la misma
	subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;//esta subpass es de graficos
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;//

	VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };//POSSIBLE BUG SOURCE
	//creamos renderPass
	VkRenderPassCreateInfo renderPassCI{};
	renderPassCI.attachmentCount = 2;
	renderPassCI.pAttachments = attachments;
	renderPassCI.pSubpasses = &subpassDescription;
	renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCI.subpassCount = 1;

	//subpass dependencies, lidian con trnasitions (especifica memory y execution dependencies entre subpasses)
	//tenemos 3 subpasses, la que creamos, la operación antes y la operación después, vulkan tiene built-in dependencies que lidian con ellas pero hay que sincronizar la de la operación después
	VkSubpassDependency subpassDependency{};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL; // VK_SUBPASS_EXTERNAL se refiere a la operación antes o después dependiendo de si está en .srcSubpass o .dstSubpass
	subpassDependency.dstSubpass = 0;//indice de subpass, en este caso la de color
	//Esperamos a:
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;//esperaremos a esta operación
	subpassDependency.srcAccessMask = 0;//específicamente a que 0 termine, osea a que 

	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;//esta operación será la que espere
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;//específicamente esperaremos hasta que acabe y luego escribiremos

	renderPassCI.dependencyCount = 1;
	renderPassCI.pDependencies = &subpassDependency;

	if (vkCreateRenderPass(logicalDevice, &renderPassCI, nullptr, &this->renderPass) != VK_SUCCESS) {
		throw std::runtime_error("could not create render pass");
	}

	//ya crear la pipeline
	VkGraphicsPipelineCreateInfo graphicsPipelineCI{};
	graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;//opcional, es para crear subpipelines que es más rapido que varias pipelines
	graphicsPipelineCI.basePipelineIndex = -1;//opcional, tiene que ver con parametro anterior
	graphicsPipelineCI.layout = this->pipelineLayout;
	graphicsPipelineCI.renderPass = this->renderPass;
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
	graphicsPipelineCI.subpass = 0;//indice

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, &this->handle) != VK_SUCCESS) {
		throw std::runtime_error("could not create graphics pipeline");
	}

	//Destroy shader modules as soon as the code is in te pipeline just like openGL
	vkDestroyShaderModule(logicalDevice, vertexShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragmentShaderModule, nullptr);
}

void asgPipeline::del() {
	vkDestroyDescriptorSetLayout(logicalDevice, this->descriptorSetLayout, nullptr);//el layout es parte de la pipeline(maso, lo necesita para crearse) pero no la pool

	vkDestroyPipeline(logicalDevice, this->handle, nullptr);
	vkDestroyRenderPass(logicalDevice, this->renderPass, nullptr);
	vkDestroyPipelineLayout(logicalDevice, this->pipelineLayout, nullptr);
}
