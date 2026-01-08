#ifndef ASG_VERTEXINDEXBUFFER_H_
#define ASG_VERTEXINDEXBUFFER_H_
#include <ASG_utils.hpp>

//local
#include <vector>
#include <ASG_vertex.hpp>

class asgVIBuffer {
	public:
		VkBuffer vertexHandle;
		VkBuffer indexHandle;

		virtual void append(std::vector<Vertex> vertexData, std::vector<uint32_t> indexData) = 0;
		virtual void del() = 0;
		void logicalClear();

		//para que no me vayan a modificar variables que necesito
		uint32_t verticesInside;
		uint32_t indicesInside;
	protected:
		VkDeviceSize vertexByteOffset;
		VkDeviceSize vertexByteSize;
		uint32_t vertexBytesUsed;

		VkDeviceSize indexByteOffset;
		VkDeviceSize indexByteSize;
		uint32_t indexBytesUsed;

		VkDeviceMemory memory;

		virtual void resize(VkDeviceSize vertexSizeRequired, VkDeviceSize indexSizeRequired) = 0;
};


class asgDeviceLocalVIB : public asgVIBuffer {
	public:

		asgDeviceLocalVIB(VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize);
		void append(std::vector<Vertex> vertexData, std::vector<uint32_t> indexData) override;
		void del() override;

	protected:

		void resize(VkDeviceSize vertexSizeRequired, VkDeviceSize indexSizeRequired) override;
};

#endif