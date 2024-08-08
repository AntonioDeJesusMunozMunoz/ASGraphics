@echo off
echo [COMPILING TEST]
glslc -c ./resourceFiles/shaderPrograms/src/testProgram.vert ./resourceFiles/shaderPrograms/src/testProgram.frag
echo [COMPILING BUFFER]
glslc -c ./resourceFiles/shaderPrograms/src/bufferShaderProgram.vert -o ./resourceFiles/shaderPrograms/compiled/bufferShaderProgram.vert.spv
glslc -c ./resourceFiles/shaderPrograms/src/bufferShaderProgram.frag -o ./resourceFiles/shaderPrograms/compiled/bufferShaderProgram.frag.spv
echo [COMPILING SPEED SHADING]
glslc -c ./resourceFiles/shaderPrograms/src/speedShading_0_1.vert -o ./resourceFiles/shaderPrograms/compiled/speedShading_0_1.vert.spv
glslc -c ./resourceFiles/shaderPrograms/src/speedShading_0_1.frag -o ./resourceFiles/shaderPrograms/compiled/speedShading_0_1.frag.spv
echo [DONE]