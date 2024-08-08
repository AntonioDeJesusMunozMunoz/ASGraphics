@echo off
echo [COMPILING SPEED SHADING]
glslc -c ./resourceFiles/shaderPrograms/src/speedShading_0_1.vert -o ./resourceFiles/shaderPrograms/compiled/speedShading_0_1.vert.spv
glslc -c ./resourceFiles/shaderPrograms/src/speedShading_0_1.frag -o ./resourceFiles/shaderPrograms/compiled/speedShading_0_1.frag.spv
echo [DONE]