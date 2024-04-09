@echo off
echo [COMPILING]
glslc -c ./resourceFiles/shaderPrograms/testProgram.vert ./resourceFiles/shaderPrograms/testProgram.frag
echo [DONE]