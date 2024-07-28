#include <ASG_modelFunctions.hpp>

//dependencies
#include <tiny_gltf.h>
#include <GLM/gtc/quaternion.hpp>

//builtin
#include <any>

//local
#include <ASG_vertex.hpp>
#include <ASG_imageHandler.hpp>

using namespace tinygltf;

std::map<int, int> numbersPerType = {
	{TINYGLTF_TYPE_SCALAR,1},
	{TINYGLTF_TYPE_VEC2, 2},
	{TINYGLTF_TYPE_VEC3, 3},
	{TINYGLTF_TYPE_VEC4, 4}
};
std::map<int, int> typeBytelength = {
	{5126, 4},
	{5125, 4},
	{5123, 2},
	{5122, 2}
};

std::vector<std::vector<std::any>>getNumbers(Model& tgModel ,Accessor accesor){
	//printf("accesor is: vec2=%d, vec3=%d, vec4=%d, scalar=%d", accesor.type == TINYGLTF_TYPE_VEC2, accesor.type == TINYGLTF_TYPE_VEC3, accesor.type == TINYGLTF_TYPE_VEC4, accesor.type == TINYGLTF_TYPE_SCALAR);
	std::vector<std::vector<std::any>> returnVector(0);
	std::vector<std::any> singleComponent(0);

	//conseguir los datos del bufferView
	BufferView bv = tgModel.bufferViews[accesor.bufferView];

	Buffer buffer = tgModel.buffers[bv.buffer];
	size_t byteOffset = accesor.byteOffset + bv.byteOffset;

	for (int i = 0; i < accesor.count; i++) {
		//creo un grupo
		for (int j = 0; j < numbersPerType[accesor.type]; j++) {

			//copio 1 elemento dependiendo del typo

			float singleElement1 = 0;
			uint32_t singleElement2 = 0;
			short singleElement3 = 0;
			unsigned short singleElement4 = 0;
			switch (accesor.componentType) {
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				//					   el inicio + offset de vecs ya leidos + offset de floats ya leidos	
				memcpy(&singleElement1, buffer.data.data() + byteOffset + i * numbersPerType[accesor.type] * typeBytelength[accesor.componentType] + j * typeBytelength[accesor.componentType], typeBytelength[accesor.componentType]);
				singleComponent.push_back(singleElement1);
				break;

			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				memcpy(&singleElement2, buffer.data.data() + byteOffset + i * numbersPerType[accesor.type] * typeBytelength[accesor.componentType] + j * typeBytelength[accesor.componentType], typeBytelength[accesor.componentType]);
				singleComponent.push_back(singleElement2);
				break;

			case TINYGLTF_COMPONENT_TYPE_SHORT:
				memcpy(&singleElement3, buffer.data.data() + byteOffset + i * numbersPerType[accesor.type] * typeBytelength[accesor.componentType] + j * typeBytelength[accesor.componentType], typeBytelength[accesor.componentType]);
				singleComponent.push_back(singleElement3);
				break;

			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				memcpy(&singleElement4, buffer.data.data() + byteOffset + i * numbersPerType[accesor.type] * typeBytelength[accesor.componentType] + j * typeBytelength[accesor.componentType], typeBytelength[accesor.componentType]);
				singleComponent.push_back(singleElement4);
				break;

			default:
				printf("hell na");
				break;
			}
		}
		
		//añado el grupo y lo vacio para el siguiente ciclo
		returnVector.push_back(singleComponent);
		singleComponent.clear();
	}
	return returnVector;
}

std::vector<Vertex> asgModelFunc::getPrimitiveVertices(tinygltf::Model& tgModel, tinygltf::Primitive primitive){

	//conseguir los atributos de vertices
	printf("accesors size: %u, primitive atributes position index:%zu", tgModel.accessors.size(), primitive.attributes["POSITION"]);
	std::vector<std::vector<std::any>> positions = getNumbers(tgModel ,tgModel.accessors[primitive.attributes["POSITION"]]);
	std::vector<std::vector<std::any>> texCoords = getNumbers(tgModel, tgModel.accessors[primitive.attributes["TEXCOORD_0"]]);
	std::vector<std::vector<std::any>> normals = getNumbers(tgModel , tgModel.accessors[primitive.attributes["NORMAL"]]);

	//armarlos y ponerlos en un vector
	std::vector<Vertex> vertices(0);

	for (int i = 0; i < positions.size(); i++) {
		Vertex currVertex;

		for (int j = 0; j < 3; j++) {
			currVertex.pos[j] = std::any_cast<float>(positions[i][j]);
			currVertex.color[j] = 1.0f;
			currVertex.normal[j] = std::any_cast<float>(normals[i][j]);
		}
		currVertex.imgPos[0] = std::any_cast<float>(texCoords[i][0]);
		currVertex.imgPos[1] = std::any_cast<float>(texCoords[i][1]);

		vertices.push_back(currVertex);
	}
	printf("\nfirst vertex data: pos= %f,%f,%f", vertices[0].pos[0], vertices[0].pos[1], vertices[0].pos[2]);
	return vertices;
}

std::vector<uint32_t> asgModelFunc::getPrimitiveIndices(tinygltf::Model& tgModel, tinygltf::Primitive primitive){
	//conseguir indices
	std::vector<std::vector<std::any>> indicesData = getNumbers(tgModel, tgModel.accessors[primitive.indices]);

	std::vector<uint32_t> indices(0);
	for (int i = 0; i < indicesData.size(); i++) {
		indices.push_back(std::any_cast<uint32_t>(indicesData[i][0]));
	}

	printf("\nfirst 3 indices: %u,%u,%u ", indices[0], indices[1], indices[2]);

	return indices;
}

asgPbrIndices asgModelFunc::loadMaterialImages(tinygltf::Model& tgModel, tinygltf::Material material)
{
	//get the image data
	Image albedo = tgModel.images[tgModel.textures[material.pbrMetallicRoughness.baseColorTexture.index].source];
	
	//put it into imageHandler and get indices
	asgPbrIndices pbrIndices;

	pbrIndices.albedoIndex = asgImageHandler::loadAlbedoMap(albedo.image, albedo.height, albedo.width, albedo.component);
	printf("\ncomponent: %i, bit depth: %i", albedo.component, albedo.bits);//bits es bit depth
	printf("\n albedo index: %u", pbrIndices.albedoIndex);

	return pbrIndices;
}

glm::mat4 asgModelFunc::getMeshTransforms(tinygltf::Model& tgModel, tinygltf::Node node)
{
	//transformation order for gltf is translation * rotation * scale (the '*' is matrix multiplication)
	//apply translation
	glm::vec3 translation;
	printf("\nnode.translation length : %zu", node.translation.size());
	for (int i = 0; i < 3; i++) {
		translation[i] = static_cast<float>(node.translation[i]);
	}
	glm::mat4 transformations = glm::translate(glm::mat4(1.0f), translation);

	//apply rotation
	glm::quat rotation(node.rotation[3], node.rotation[1], node.rotation[2], node.rotation[0]);
	transformations *= glm::mat4_cast(rotation);
	
	return transformations;
}
