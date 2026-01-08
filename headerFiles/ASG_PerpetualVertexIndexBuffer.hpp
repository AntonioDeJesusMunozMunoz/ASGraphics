#ifndef ASG_PERPETUAL_VERTEXINDEXBUFFER_H_
#define ASG_PERPETUAL_VERTEXINDEXBUFFER_H_
#include <ASG_VertexIndexBuffer.hpp>

/*perpetually mapped vertex index buffer, faster if im constantly sending data, slower if the data gets sent rarely*/
class asgPerpetualVIB: public asgVIBuffer {
	
	public:
		asgPerpetualVIB(uint32_t vertexBufferSize, uint32_t indexBufferSize);
		void append(std::vector<Vertex> vertexData, std::vector<uint32_t> indexData) override;
		void del() override;

	protected:
		void* mappedVertexBufferStart;
		void* mappedIndexBufferStart;
		void resize(VkDeviceSize vertexSizeRequired, VkDeviceSize indexSizeRequired) override;


};
#endif