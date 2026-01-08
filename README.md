# **ASGraphics**
Un motor de gráficos con interfáz de alto nivel hecho con vulkan en c++.
El motor de gráficos maneja la carga de modelos, la memoria y la renderización usando la técnica de "deferred rendering"

El proyecto es principalmente para uso personal.

## **Librerias**
Se usan las siguientes librerias:
- Vulkan (El sdk de https://vulkan.lunarg.com/)
- GLFW (https://github.com/g-truc/glm)
- GLM  (https://github.com/g-truc/glm)
- STB  (https://github.com/nothings/stb)
- HashLibrary (https://github.com/stbrumme/hash-library)
- tinyGLTF  (https://github.com/syoyo/tinygltf)

## **Filosofía de diseño**
ASGraphics está diseñada para una integración profunda con los proyectos que la utilicen, permitiendo:
- Optimizaciones agresivas
- Acceso directo a estructuras internas cuando es necesario
- Mantener una interfaz simple y de alto nivel

Debido a esta integración profunda, el uso de la librería requiere una estructura de archivos específica. Esta estructura es la siguiente:

- tener en dependencies/include/: GLM(carpeta de proyecto, usandolo como submodulo de git), vulkan(carpeta de vulkan dentro del sdk, la que tiene la carpeta vk_video y vulkan), ASGraphics.hpp y ASG_vertex.hpp.
- tener en dependencies/lib/:  glfw3.lib, vulkan-1.lib (del sdk), ASGraphics.lib. (La libreria de ASGraphics se puede generar usando Cmake) 
- tener en resourceFiles/nonModelImages/:  lightingThresholds.jpg
- tener en resourceFiles/shaders/compiled:  gBufferPass.vert, gBufferPass.frag, lightingPass.vert, y lightingPass.frag
 
*El path en el que se espera la carpeta de resourceFiles se puede cambiar llamando la funcion "asgConfigChangeResourceFilesPath" de asgConfig ANTES de llamar asgInit para inicializa la libreria 

*El proyecto debe ser compilado con MSVC porque con ese se compiló la libreria

## **Explicaciones extra**
- Los archivos .bat son para compilar diferentes versiones de los shaders.
- Este proyecto NO es para simple implementación con proyectos existentes.
- Se publica con fines de revisión de código y referencia de arquitectura.

