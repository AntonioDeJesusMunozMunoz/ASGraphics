#ifndef ASG_GRAPHICS_PIPELINE_H_
#define ASG_GRAPHICS_PIPELINE_H_
#include <ASG_utils.hpp>

//dependencies

//builtin

//local

//structs
struct asgPipeline {
	VkPipeline handle;
	VkDescriptorSetLayout descriptorSetLayout;//de momento solo lo usa graphicsPipeline(lo crea y lo usa) y main(solo lo usa)
	VkPipelineLayout pipelineLayout;

	//método
	void del();
};

//func
namespace asgPipelineFunc {
	//fills
	VkPipelineMultisampleStateCreateInfo fillNoMultisampleCI();
	std::vector<VkVertexInputAttributeDescription> fillDefaultVertexInputAttrDescs();
	VkPipelineDynamicStateCreateInfo fillDynamicStateCI(std::vector<VkDynamicState>* dynamicStates);
	VkPipelineShaderStageCreateInfo fillShaderStageCI(VkShaderStageFlagBits shaderStage, const char* entryPoint, VkShaderModule module);
	VkPipelineRasterizationStateCreateInfo fillDefaultRasterizationCI();
	VkPipelineColorBlendStateCreateInfo fillColorBlendCI(std::vector<VkPipelineColorBlendAttachmentState>* blendAttachments, std::vector<float> fourBlendConstants, VkBool32 enableLogicOp = VK_FALSE, VkLogicOp op = VK_LOGIC_OP_COPY);
	
	//creates
	VkDescriptorSetLayout createDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> layoutBindings);
	VkDescriptorSetLayoutBinding createLayoutBinding(uint32_t bindingIndex, uint32_t descriptorCount, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, VkSampler* pInmutableSamplers = nullptr);
	VkShaderModule createShaderModule(std::vector<unsigned char> rawDataVector);
}
#endif