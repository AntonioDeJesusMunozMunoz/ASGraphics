#include <binFileLoader.hpp>
#ifndef NDEBUG
#include <iostream>
#endif

std::vector<unsigned char> readRawBinary(std::string path){

#ifndef NDEBUG
		std::cout << "reading raw binary file: " << path << std::endl;
#endif

    //open file  (a)t (t)he (e)nd   in binary           //std::cout << "trying to read file: " << path << std::endl;
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()){

#ifndef NDEBUG
		std::cout << "could not open file: " << path << std::endl;
#endif

        throw std::runtime_error("could not open file");
    }

    //resize vector
    size_t fileSize = (size_t)file.tellg();
    std::vector<unsigned char> rawData(fileSize);

    //read
    file.seekg(0);
    file.read((char *)rawData.data(), fileSize);

    //exit
    file.close();

#ifndef NDEBUG
    std::cout << "done reading" << std::endl;
#endif

    return rawData;
}