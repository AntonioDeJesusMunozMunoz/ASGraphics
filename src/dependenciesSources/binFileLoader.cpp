#include <binFileLoader.hpp>
#include <windows.h>

std::vector<unsigned char> readRawBinary(std::string path){
    //open file  (a)t (t)he (e)nd   in binary
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()){
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

    return rawData;
}