#include <ASG_model.hpp>

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
	return vertices;
}

std::vector<uint32_t> asgModelFunc::getPrimitiveIndices(tinygltf::Model& tgModel, tinygltf::Primitive primitive){
	//conseguir indices
	std::vector<std::vector<std::any>> indicesData = getNumbers(tgModel, tgModel.accessors[primitive.indices]);

	//pasarlos a un vector
	/*std::map<std::type_info, int> typeBytelength = {
	{typeid(float), 5126},
	{typeid(unsigned int), 5125},
	{typeid(unsigned short), 5123},
	{typeid(short), 5122}
	};*/

	std::map<std::string, int> typeValue = {
	{"float", 5126},
	{"unsigned int", 5125},
	{"unsigned short", 5123},
	{"short", 5122}
	};

	std::vector<uint32_t> indices(0);
	for (int i = 0; i < indicesData.size(); i++) {
		switch (typeValue[std::string(indicesData[i][0].type().name())]) {
			case 5126:
				throw std::runtime_error("indices as float????");
				break;

			case 5125:
				indices.push_back(static_cast<uint32_t>(std::any_cast<unsigned int>(indicesData[i][0])));
				break;

			case 5123:
				indices.push_back(static_cast<uint32_t>(std::any_cast<unsigned short>(indicesData[i][0])));
				break;

			case 5122:
				indices.push_back(static_cast<uint32_t>(std::any_cast<short>(indicesData[i][0])));
				break;

			default:
				printf("tipo de indice: %u", typeValue[std::string(indicesData[i][0].type().name())]);
				throw std::runtime_error("aparecio un tipo de indice no supported por el switch");
		}
	}

	return indices;
}

asgPbrIndices asgModelFunc::loadMaterialImages(tinygltf::Model& tgModel, tinygltf::Material material)
{	
	//get the image data
	Image albedo = tgModel.images[tgModel.textures[material.pbrMetallicRoughness.baseColorTexture.index].source];
	
	//put it into imageHandler and get indices
	asgPbrIndices pbrIndices;

	pbrIndices.albedoIndex = asgImageHandler::loadAlbedoMap(albedo.image, albedo.height, albedo.width, albedo.component);
	if (validationLayersEnabled) {
		printf("\ncomponent: %i, bit depth: %i", albedo.component, albedo.bits);//bits es bit depth
	}

	return pbrIndices;
}

glm::mat4 asgModelFunc::getMeshTransforms(tinygltf::Model& tgModel, tinygltf::Node node)
{
	
	//transformation order for gltf is translation * rotation * scale (the '*' is matrix multiplication)
	//get translation
	glm::vec3 translation(0.0f,0.0f,0.0f);//default
	for (int i = 0; i < node.translation.size(); i++) {
		translation[i] = static_cast<float>(node.translation[i]);
	}
	glm::mat4 transformations = glm::translate(glm::mat4(1.0f), translation);

	//get rotation
	glm::quat rotation(0.0f, 0.0f, 0.0f, 0.0f);//default
	if (node.rotation.size() == 4) {//quaternion
		rotation.w = static_cast<float>(node.rotation[3]);
		rotation.x = static_cast<float>(node.rotation[0]);
		rotation.y = static_cast<float>(node.rotation[1]);
		rotation.z = static_cast<float>(node.rotation[2]);
	}
	else if (node.rotation.size() == 3) {//euler angles
		throw std::runtime_error("node has rotation as euler angles (not supported)");
	}

	//get scale
	glm::vec3 scale(1.0f, 1.0f, 1.0f);//default
	for (int i = 0; i < node.scale.size(); i++) {
		scale[i] = static_cast<float>(node.scale[i]);
	}
	glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);

	//get combined transformations
	transformations *= glm::mat4_cast(rotation);
	transformations *= scaleMat;
	
	return transformations;
}

//class method definitions
asgPrimitive::asgPrimitive(tinygltf::Model& tgModel, tinygltf::Primitive& tgPrimitive)
{
	//get material
	tinygltf::Material tgMaterial = tgModel.materials[0];//default
	if (tgPrimitive.material != -1) {
		tgMaterial = tgModel.materials[tgPrimitive.material];
	}

	//make material stuff if there isnt any
	if (materialsBuffers.find(tgMaterial.name) == materialsBuffers.end()) {//si no esta
		materialsBuffers.insert(std::make_pair(tgMaterial.name, std::make_unique<asgVIBuffer>(1000, 1000)));
		materialsPrimitives.insert(std::make_pair(tgMaterial.name, std::vector<asgPrimitive*>(0)));
	}

	//make variables
	std::vector<Vertex> vertices = asgModelFunc::getPrimitiveVertices(tgModel, tgPrimitive);
	std::vector<uint32_t> indices = asgModelFunc::getPrimitiveIndices(tgModel, tgPrimitive);
 
	//initialize atributes
	this->currTransform = glm::mat4(1.0f);

	this->indexCount = static_cast<uint32_t>(indices.size());
	this->indexOffset = materialsBuffers[tgMaterial.name]->indicesInside;
	this->vertexOffset = materialsBuffers[tgMaterial.name]->verticesInside;

	this->matrixIndex = 0;
	this->pbrIndices = asgModelFunc::loadMaterialImages(tgModel, tgMaterial);
	this->toBeDrawn = false;

	this->materialName = tgMaterial.name;

	//put my data into the buffer
	materialsBuffers[tgMaterial.name]->append(vertices, indices);

}

asgMesh::asgMesh(tinygltf::Model& tgModel, tinygltf::Mesh tgMesh)
{
	//fill primitives
	for (auto& tgPrimitive : tgMesh.primitives) {
		//make primitive
		asgPrimitive currPrimitive(tgModel, tgPrimitive);
		//append
		this->primitives.push_back(currPrimitive);
	}
}
void asgMesh::applyMat(glm::mat4 matrix)
{
	for (auto& primitive : primitives) {
		primitive.currTransform = matrix;
	}
}


asgModelNode::asgModelNode(tinygltf::Model& tgModel, asgModel *theModel, tinygltf::Node& tgNode, glm::mat4 parentTransform)
{
	//fill transforms
	this->currParentTransform = parentTransform;
	this->defaultTransform = asgModelFunc::getMeshTransforms(tgModel, tgNode);

	//fill mesh
	if (tgNode.mesh != -1) {//si tiene mesh
		this->hasMesh = true;
		this->mesh = &theModel->meshes[tgNode.mesh];
	}
	else {//si no tiene mesh
		this->hasMesh = false;
		this->mesh = nullptr;
	}

	//fill children vector
	for (auto& tgChildrenNodeIndex : tgNode.children) {
		//make child
		asgModelNode currChild(tgModel, theModel, tgModel.nodes[tgChildrenNodeIndex], this->currParentTransform * this->defaultTransform);
		//append child
		this->children.push_back(currChild);
	}
}
void asgModelNode::appplyMat(glm::mat4 matrix)
{
	//update my current transform
	currParentTransform = matrix;

	glm::mat4 combinedTransform = currParentTransform * defaultTransform;
	////update the mesh primitives transform if it has a mesh
	if (this->hasMesh) {
		mesh->applyMat(combinedTransform);
	}

	for (auto& child : children) {
		//update the childrens transforms
		child.appplyMat(combinedTransform);
	}
}

asgModel::asgModel(std::string path, tinygltf::Model& tgModel)
{
	//fill my atributes
		//fill my path
	this->path = path;

	//for mesh in tgModel
	for (auto& tgMesh : tgModel.meshes) {
		//make mesh
		asgMesh currMesh(tgModel, tgMesh);
		//append to my meshes
		this->meshes.push_back(currMesh);
	}

	//get my mesh primitives into the map
	for (int i = 0; i < this->meshes.size(); i++) {
		for (int j = 0; j < this->meshes[i].primitives.size(); j++) {
			materialsPrimitives[this->meshes[i].primitives[j].materialName].push_back(&this->meshes[i].primitives[j]);
		}
	}

	//for node in tgModel
	for (auto& tgNode : tgModel.nodes) {
		//make node
		asgModelNode currNode(tgModel, this, tgNode, glm::mat4(1.0f));
		//append to my nodes
		this->rootNodes.push_back(currNode);
	}
}
void asgModel::applyMat(glm::mat4 modelMatrix)
{
	for (auto& node : rootNodes) {
		node.appplyMat(modelMatrix);
	}
}