[Español](README.es.md)

# **ASGraphics**
A graphics engine with a high-level interface, built with Vulkan in C++.
The engine handles model loading, memory management, and rendering using the "deferred rendering" technique.

This project is mainly for personal use.

## **Libraries**
The following libraries are used:
- Vulkan (the SDK from https://vulkan.lunarg.com/)
- GLFW (https://github.com/glfw/glfw)
- GLM (https://github.com/g-truc/glm)
- STB (https://github.com/nothings/stb)
- HashLibrary (https://github.com/stbrumme/hash-library)
- tinyGLTF (https://github.com/syoyo/tinygltf)

## **Design philosophy**
ASGraphics is designed for deep integration with the projects that use it, allowing for:
- Aggressive optimizations
- Direct access to internal structures when needed
- A simple, high-level interface

Because of this deep integration, using the library requires a specific file structure, as follows:

- in `dependencies/include/`: GLM (project folder, used as a git submodule), vulkan (the folder from the SDK containing the `vk_video` and `vulkan` folders), `ASGraphics.hpp`, and `ASG_vertex.hpp`.
- in `dependencies/lib/`: `glfw3.lib`, `vulkan-1.lib` (from the SDK), and `ASGraphics.lib` (the ASGraphics library can be generated using CMake).
- in `resourceFiles/nonModelImages/`: `lightingThresholds.jpg`.
- in `resourceFiles/shaders/compiled`: `gBufferPass.vert`, `gBufferPass.frag`, `lightingPass.vert`, and `lightingPass.frag`.

*The expected path for the `resourceFiles` folder can be changed by calling `asgConfigChangeResourceFilesPath` from `asgConfig` BEFORE calling `asgInit` to initialize the library.*

*The project must be compiled with MSVC, since that's what the library was compiled with.*

## **Additional notes**
- The `.bat` files are used to compile different versions of the shaders.

## **Notice**
- This project is NOT intended for straightforward integration into existing projects.
- It is published for code review and architecture reference purposes.
