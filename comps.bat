@echo off
echo [COMPILING TEST]
glslc -c ./resourceFiles/shaderPrograms/testProgram.vert ./resourceFiles/shaderPrograms/testProgram.frag
echo [COMPILING BUFFER]
glslc -c ./resourceFiles/shaderPrograms/bufferShaderProgram.vert ./resourceFiles/shaderPrograms/bufferShaderProgram.frag
echo [DONE]