#ifndef ASG_GRAPHICS_PIPELINE_H
#define ASG_GRAPHICS_PIPELINE_H
#include <ASG_utils.hpp>

//dependencies

//builtin

//local

/*variables*/


/*structs*/
struct asgPipeline {
	VkPipeline handle;
	VkDescriptorSetLayout descriptorSetLayout;//de momento solo lo usa graphicsPipeline(lo crea y lo usa) y main(solo lo usa)
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;

	//método
	asgPipeline(VkFormat chosenSwapSurfaceFormat, VkFormat supportedDepthBufferFormat);
	void del();
};

/*funciones*/
#endif