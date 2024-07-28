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

	asgVIBuffer(VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize);

	void append(std::vector<Vertex> vertexData, std::vector<uint32_t> indexData);

	void del();

	//para que no me vayan a modificar variables que necesito
	uint32_t verticesInside;
	uint32_t indicesInside;

private:
	VkDeviceSize vertexByteOffset;
	VkDeviceSize vertexByteSize;
	uint32_t vertexBytesUsed;

	VkDeviceSize indexByteOffset;
	VkDeviceSize indexByteSize;
	uint32_t indexBytesUsed;

	VkDeviceMemory memory;

	void resize(VkDeviceSize vertexSizeRequired, VkDeviceSize indexSizeRequired);
};
#endif