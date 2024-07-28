#ifndef ASG_IMAGE_HANDLER_H_
#define ASG_IMAGE_HANDLER_H_

//builtins
#include <vector>
#include <memory>

//locals
#include <ASG_utils.hpp>

//single memory allocation, an imageArray of lenght numOfMaterials and one for each pbr image type, it would have an append a resize, a del and a check for if
//an image is already loaded

//TODO IDENTIFIED PROBLEMS
//if an image need memoryType 1, and i have a memory of that type, it will use it, REGARDLESS OF IT HAVING ENOUGH SIZE OR NOT
	//could be solved by vma implementation?
namespace asgImageHandler {
		//funciones
		void initializeResources();
		uint32_t loadAlbedoMap(std::string path);
		uint32_t loadAlbedoMap(std::vector<unsigned char> data, uint32_t imageTall, uint32_t imageLong,uint32_t  numCollChannells);
		void deleteResources();
		
};
#endif