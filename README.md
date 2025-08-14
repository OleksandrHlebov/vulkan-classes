[![CMake Windows Build with Vulkan SDK](https://github.com/OleksandrHlebov/vulkan-classes/actions/workflows/main.yml/badge.svg?branch=master)](https://github.com/OleksandrHlebov/vulkan-classes/actions/workflows/main.yml)

# Vulkan classes library

Contains [VulkanAPI](https://www.vulkan.org/) native classes wrappers using modern C++, enhancing the ease of use.

Made with Vulkan 1.3 in mind, not tested with Vulkan 1.4.
Uses [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) and [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap) for reducing the amount of boilerplate code required for setup and better memory management.
[GLFW](https://github.com/glfw/glfw) is also included as main solution for window creation. 

For examples of use take a look at [my renderer](https://github.com/OleksandrHlebov/VulkanRenderer) using this library, including a minimal triangle drawing project.
