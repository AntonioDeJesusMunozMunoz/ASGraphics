@echo off
echo [COMPILING TEST]
glslc -c ./resourceFiles/shaderPrograms/src/testProgram.vert ./resourceFiles/shaderPrograms/src/testProgram.frag
echo [COMPILING BUFFER]
glslc -c ./resourceFiles/shaderPrograms/src/bufferShaderProgram.vert ./resourceFiles/shaderPrograms/src/bufferShaderProgram.frag
echo [DONE]