#ifndef ASG_RENDER_PASS_H_
#define ASG_RENDER_PASS_H_

#include <map>

#include <ASG_utils.hpp>
#include <ASG_swapChain.hpp>
#include <ASG_graphicsPipeline.hpp>

struct asgRenderSubPass {//just a grouping of renderSubpass stuff
	VkSubpassDescription description;
	asgPipeline pipeline;
	std::vector<VkDescriptorSet> descriptorSets;
	void fillSubPassDescription(std::vector<VkAttachmentReference> *attachmentRef, VkAttachmentReference* depthReference, std::vector<VkAttachmentReference>* inputAttachmentRef);
};

struct asgRenderPass {//just grouping a bunch of related stuff together, no constructor bc each renderPass will be different
	//var
	VkRenderPass handle;
	VkRenderPassBeginInfo beginInfo;
	std::vector<VkFramebuffer> framebuffers;
	std::vector<VkImage>attachmentImages;
	std::vector<VkImageView>attachmentImageViews;
	VkDeviceMemory attachmentMemory;//TODO
	std::vector<asgRenderSubPass> subPasses;

	//func
	void createFrameBuffers(asgSwapChain& sc);
	//VkRenderPassBeginInfo fillBeginInfo(uint32_t swapChainImageIndex);//TODO?
	void destroyFrameBuffers();
	void del();

	//func pointers
	void (*_createFrameBuffersFunc)(asgSwapChain&, asgRenderPass*);
};

//this one has no namespace to test if i need them
//variables
extern std::vector<asgRenderPass>renderPasses;

//func
void createRenderPasses(asgSwapChain sc);

#endif