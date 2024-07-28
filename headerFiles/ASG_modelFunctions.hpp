#ifndef ASG_MODEL_LOADER_H_
#define ASG_MODEL_LOADER_H_

//dependencies
#include <GLM/glm.hpp>
#include <GLM/matrix.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <tiny_gltf.h>

//builtin
#include <vector>

//local
#include <ASG_utils.hpp>
#include <ASG_vertex.hpp>

namespace asgModelFunc {
	std::vector<Vertex> getPrimitiveVertices(tinygltf::Model& tgModel, tinygltf::Primitive primitive);
	std::vector<uint32_t> getPrimitiveIndices(tinygltf::Model& tgModel, tinygltf::Primitive primitive);
	asgPbrIndices loadMaterialImages(tinygltf::Model& tgModel, tinygltf::Material material);
	glm::mat4 getMeshTransforms(tinygltf::Model& tgModel, tinygltf::Node node);
}
#endif 