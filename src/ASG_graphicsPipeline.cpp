#include <ASG_graphicsPipeline.hpp>
//dependencies

//builtin
#include <vector>
//local
#include <dependencies/binFileLoader.hpp>
#include <ASG_vertex.hpp>

//definiciones
VkShaderModule asgPipelineFunc::createShaderModule(std::vector<unsigned char> rawDataVector) {

	if (validationLayersEnabled) {
		printf("creating a shader module\n");
	}

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

VkDescriptorSetLayoutBinding asgPipelineFunc::createLayoutBinding(uint32_t bindingIndex, uint32_t descriptorCount, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, VkSampler* pInmutableSamplers) {
	VkDescriptorSetLayoutBinding returnLB{};
	returnLB.binding = bindingIndex;
	returnLB.descriptorCount = descriptorCount;
	returnLB.descriptorType = descriptorType;
	returnLB.stageFlags = stageFlags;
	returnLB.pImmutableSamplers = pInmutableSamplers;

	return returnLB;
}

VkDescriptorSetLayout asgPipelineFunc::createDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> layoutBindings) {
	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	createInfo.pBindings = layoutBindings.data();

	VkDescriptorSetLayout returnLayout;
	if (vkCreateDescriptorSetLayout(logicalDevice, &createInfo, nullptr, &returnLayout) != VK_SUCCESS) {
		throw std::runtime_error("could not create descriptor set layout");
	}

	return returnLayout;
}

VkPipelineShaderStageCreateInfo asgPipelineFunc::fillShaderStageCI(VkShaderStageFlagBits shaderStage, const char* entryPoint, VkShaderModule module) {
	VkPipelineShaderStageCreateInfo returnCI{};
	returnCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	returnCI.module = module;
	returnCI.stage = shaderStage;
	returnCI.pName = entryPoint;
	
	return returnCI;
}

VkPipelineDynamicStateCreateInfo asgPipelineFunc::fillDynamicStateCI(std::vector<VkDynamicState> *dynamicStates) {
	VkPipelineDynamicStateCreateInfo returnCI{};
	returnCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	returnCI.dynamicStateCount = static_cast<uint32_t>(dynamicStates->size());
	returnCI.pDynamicStates = dynamicStates->data();

	return returnCI;
}

std::vector<VkVertexInputAttributeDescription> asgPipelineFunc::fillDefaultVertexInputAttrDescs() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);
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

	return attributeDescriptions;
}

VkPipelineMultisampleStateCreateInfo asgPipelineFunc::fillNoMultisampleCI() {
	VkPipelineMultisampleStateCreateInfo multisampleCI{};
	multisampleCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCI.alphaToCoverageEnable = VK_FALSE;
	multisampleCI.alphaToOneEnable = VK_FALSE;
	multisampleCI.sampleShadingEnable = VK_FALSE;
	multisampleCI.minSampleShading = 1.0f;
	multisampleCI.pSampleMask = nullptr;
	multisampleCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	return multisampleCI;
}

VkPipelineRasterizationStateCreateInfo asgPipelineFunc::fillDefaultRasterizationCI() {
	VkPipelineRasterizationStateCreateInfo rasterizationCI{};
	rasterizationCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationCI.depthClampEnable = VK_FALSE;//clampea lo que se sale del frustum en vez de cortarlo
	rasterizationCI.rasterizerDiscardEnable = VK_FALSE;//la geometria nunca deja el resterizador
	rasterizationCI.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationCI.lineWidth = 1.0f;//medido en fragmentos
	rasterizationCI.cullMode = VK_CULL_MODE_NONE; //VK_CULL_MODE_BACK_BIT;//TODO
	rasterizationCI.frontFace = VK_FRONT_FACE_CLOCKWISE;

	rasterizationCI.depthBiasEnable = VK_FALSE;
	rasterizationCI.depthBiasClamp = 0.0f;
	rasterizationCI.depthBiasConstantFactor = 0.0f;
	rasterizationCI.depthBiasSlopeFactor = 0.0f;

	return rasterizationCI;
}

VkPipelineColorBlendStateCreateInfo asgPipelineFunc::fillColorBlendCI(std::vector<VkPipelineColorBlendAttachmentState> *blendAttachments, std::vector<float> fourBlendConstants, VkBool32 enableLogicOp, VkLogicOp op) {
	VkPipelineColorBlendStateCreateInfo blendStateCI{};
	blendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendStateCI.pAttachments = blendAttachments->data();
	blendStateCI.attachmentCount = static_cast<uint32_t>(blendAttachments->size());
	blendStateCI.logicOpEnable = enableLogicOp;//tambien podr�as blendear con bitwise ops, pero poner esto true hace el attachment false
	blendStateCI.logicOp = op;
	blendStateCI.blendConstants[0] = fourBlendConstants[0];
	blendStateCI.blendConstants[1] = fourBlendConstants[1];
	blendStateCI.blendConstants[2] = fourBlendConstants[2];
	blendStateCI.blendConstants[3] = fourBlendConstants[3];

	return blendStateCI;
}

void asgPipeline::del() {
	vkDestroyDescriptorSetLayout(logicalDevice, this->descriptorSetLayout, nullptr);//el layout es parte de la pipeline(maso, lo necesita para crearse) pero no la pool

	vkDestroyPipeline(logicalDevice, this->handle, nullptr);
	vkDestroyPipelineLayout(logicalDevice, this->pipelineLayout, nullptr);
}
