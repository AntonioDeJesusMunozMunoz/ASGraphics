#ifndef ASG_MODEL_H_
#define ASG_MODEL_H_

//dependencies
#include <GLM/glm.hpp>
#include <GLM/matrix.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <tiny_gltf.h>

//builtin
#include <vector>

//local
#include <ASG_utils.hpp>
#include <ASG_VertexIndexBuffer.hpp>
#include <ASG_vertex.hpp>

//classes and structs

//declarations
struct asgPrimitive;
struct asgMesh;
struct asgModelNode;
struct asgModel;

//definitions

struct asgPrimitive {//deberia de hacer esto una clase y poner pbrIndices, indexOffset, vertexOffset, indexCount y currTransform tras getters, pero me da hueva
	asgPbrIndices pbrIndices;
	uint32_t indexOffset;
	uint32_t vertexOffset;
	uint32_t indexCount;
	glm::mat4 currTransform;

	uint32_t matrixIndex;
	bool toBeDrawn;
	std::string materialName;

	asgPrimitive(tinygltf::Model& tgModel, tinygltf::Primitive& tgPrimitive);
};

struct asgMesh {
	std::vector<asgPrimitive>primitives;
	void applyMat(glm::mat4 matrix);

	asgMesh(tinygltf::Model& tgModel, tinygltf::Mesh tgMesh);
};

struct asgModelNode {
	public:
		std::vector<asgModelNode>children;

		asgModelNode(tinygltf::Model& tgModel, asgModel* theModel, tinygltf::Node& tgNode, glm::mat4 parentTransform);
		void appplyMat(glm::mat4 matrix);

	private:
		glm::mat4 defaultTransform;
		glm::mat4 currParentTransform;
		asgMesh* mesh;
		bool hasMesh;
};

struct asgModel {
	std::string path;
	std::vector<asgMesh> meshes;
	std::vector<asgModelNode>rootNodes;

	asgModel(std::string path, tinygltf::Model& tgModel);
	void applyMat(glm::mat4 modelMatrix);
};

//externs
extern std::map<std::string, std::unique_ptr<asgVIBuffer>> materialsBuffers;
extern std::map<std::string, std::vector<asgPrimitive*>> materialsPrimitives;

//func
namespace asgModelFunc {
	std::vector<Vertex> getPrimitiveVertices(tinygltf::Model& tgModel, tinygltf::Primitive primitive);
	std::vector<uint32_t> getPrimitiveIndices(tinygltf::Model& tgModel, tinygltf::Primitive primitive);
	asgPbrIndices loadMaterialImages(tinygltf::Model& tgModel, tinygltf::Material material);
	glm::mat4 getMeshTransforms(tinygltf::Model& tgModel, tinygltf::Node node);
}
#endif 