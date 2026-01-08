#ifndef ASG_CONFIG_H
#define ASG_CONFIG_H
#include <iostream>
#include <string>

extern std::string resourceFilesPath;

void asgConfigChangeResourceFilesPath(std::string newPath);
#endif