#include <ASG_renderPass.hpp>

//local

//sub local
#include <ASG_renderPass_deffered.hpp>

std::vector<asgRenderPass>renderPasses;

//public
void createRenderPasses(asgSwapChain sc) {
	renderPasses.push_back(createDefferedRenderingPass(sc));
	//renderPasses.push_back(createPostProcessingPass(sc));
}


//methods
void asgRenderPass::createFrameBuffers(asgSwapChain& sc){
	this->_createFrameBuffersFunc(sc, this);
}

void asgRenderPass::destroyFrameBuffers()
{
	for (auto currFrameBuffer : this->framebuffers) {//debemos destruirlo antes de la render pass e image views
		vkDestroyFramebuffer(logicalDevice, currFrameBuffer, nullptr);
	}
}

void asgRenderPass::del(){
	//destroy framebuffers
	this->destroyFrameBuffers();//frameBuffers se deben destruir antes de render pass e image views

	//destroy images
	for (auto& image : this->attachmentImages) {
		vkDestroyImage(logicalDevice, image, nullptr);
	}

	//destroy imageViews
	for (auto& imageView : this->attachmentImageViews) {
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	}

	//destroy handle
	vkDestroyRenderPass(logicalDevice, this->handle, nullptr);

	for (auto& subPass : this->subPasses) {
		subPass.pipeline.del();
	}

	//free memory
	vkFreeMemory(logicalDevice, this->attachmentMemory, nullptr);
}

void asgRenderSubPass::fillSubPassDescription(std::vector<VkAttachmentReference>* attachmentRef, VkAttachmentReference* depthReference, std::vector<VkAttachmentReference>* inputAttachmentRef){
	VkSubpassDescription subPassDescription{};
	subPassDescription.colorAttachmentCount = static_cast<uint32_t>(attachmentRef->size());
	subPassDescription.pColorAttachments = attachmentRef->data();
	subPassDescription.pDepthStencilAttachment = depthReference;
	subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPassDescription.pInputAttachments = inputAttachmentRef->data();
	subPassDescription.inputAttachmentCount = static_cast<uint32_t>(inputAttachmentRef->size());

	this->description = subPassDescription;
}
