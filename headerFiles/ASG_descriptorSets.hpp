/*a look into the descriptorSets references*/
#ifndef ASG_DESCRIPTOR_SETS_H_
#define ASG_DESCRIPTOR_SETS_H_

//builtin
#include <vector>

//local
#include <ASG_utils.hpp>
#include <ASG_graphicsPipeline.hpp>

//extern VkDeviceMemory matrixUniformMemory;
extern std::vector<void*> mappedUniformBufferMemories;
extern std::vector<VkDescriptorSet> descriptorSets;

extern std::vector<void*> mappedMeshModelMatricesBuffers;

class descriptorSetImageUpdater {
public:
	virtual void addAlbedoMap(VkImageView albedoMapImageView) {
	}

	virtual void del() {
	}
};

class dsImageUpdater1 : public descriptorSetImageUpdater {
public:
	void addAlbedoMap(VkImageView albedoMapImageView) override;
	
	dsImageUpdater1();
	void del();
private:
	VkSampler albedoSampler;
};

void initializeDescriptorSets(asgPipeline* graphicsPipeline);//i abstracted this away to gatekeep the descriptorSets handles
void destroyDescriptorSetResources();//and this just to keep the descriptorPool and others encapsulated
#endif
/*
PROBLEMA: como actualizo los descriptor sets

S ingle responsability
O pen for extension / closed for modification
L liskov sus : deberias poder sustituir una clase por cualquiera de sus hijos sin que deje de ser correcto el programa, esto te sirve como un check para q tan bien vas
I nterface segregation : interface puede ser una clase, no te muestres cosas en una clase que no vas a necesitar
D ependency inversion : que clases no dependan directamente de otros, sinó de una abstracción(abc class) de otro

//idea: una función que actualize los descriptor sets
	//AñadirAlbedo(imageView)
		//S: solo hace una cosa [o]
		//O: si quiero más, añado más funciones, pero no modifico esta [o]
		//L: no es una clase [~]
		//I: solo necesito añadir albedo, así que solo pido el imageView [oo]
		//D: dependo directamente de una función y no de una abstraccion de esta [X]
			//clase abc que encapsule como una posible función se podría ver
				//actualizadorDeImagenesEnDescriptorSets{
	virtual void addAlbedo();
//mas luego
				}
*/
