#ifndef ASG_RENDER_PASS_DEFFERED_H_
#define ASG_RENDER_PASS_DEFFERED_H_

#include <ASG_utils.hpp>
#include <ASG_swapChain.hpp>
#include <ASG_renderPass.hpp>

asgRenderPass createDefferedRenderingPass(asgSwapChain& sc);

namespace defferedPassFunc {
	namespace gBufferSubPass {
		//descriptors
		extern VkDescriptorPool descriptorPool;

		//uniforms
		extern std::vector<VkBuffer>uniformBuffers;
		extern VkDeviceMemory matrixUniformMemory;
		extern std::vector<void*> mappedUniformBufferMemories;

		extern std::vector<VkBuffer>meshModelMatricesBuffers;
		extern VkDeviceMemory meshModelMatricesMemory;
		extern std::vector<void*> mappedMeshModelMatricesBuffers;

		extern uint32_t amountOfLoadedAlbedos;//hiba a pensar en como lidiar con unloading de imageViews, pero nada del código lidia con eso así q ps así lo dejo y cuando lidie con unloading lo hago todo junto 

		//constant uniforms
		extern VkDeviceMemory constantUniformsMemory;

		//lightingThresholds variables
		extern VkImage lightingThresholds;
		extern VkImageView lightingThresholdsIV;
		extern VkSampler lightingThresholdsSampler;

		//func
		void destroyDescriptorSetResources();
	}

	//ligthing subpass
	namespace ligthingSubPass {
		void destroyDescriptorSetResources();
	}
}
#endif