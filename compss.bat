@echo off
REM echo [COMPILING SPEED SHADING]
REM glslc -c ./resourceFiles/shaderPrograms/src/speedShading_0_1.vert -o ./resourceFiles/shaderPrograms/compiled/speedShading_0_1.vert.spv
REM glslc -c ./resourceFiles/shaderPrograms/src/speedShading_0_1.frag -o ./resourceFiles/shaderPrograms/compiled/speedShading_0_1.frag.spv
echo [COMPILING GBUFFER SHADING]
glslc -c ./resourceFiles/shaderPrograms/src/gBufferPass.vert -o ./resourceFiles/shaderPrograms/compiled/gBufferPass.vert.spv
glslc -c ./resourceFiles/shaderPrograms/src/gBufferPass.frag -o ./resourceFiles/shaderPrograms/compiled/gBufferPass.frag.spv
echo [COMPILING LIGTHING SHADING]
glslc -c ./resourceFiles/shaderPrograms/src/lightingPass.vert -o ./resourceFiles/shaderPrograms/compiled/lightingPass.vert.spv
glslc -c ./resourceFiles/shaderPrograms/src/lightingPass.frag -o ./resourceFiles/shaderPrograms/compiled/lightingPass.frag.spv
echo [DONE]
