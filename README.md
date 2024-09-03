# Compiling
## Prerequisites
* GLFW
* FMT
* Vulkan
* Clang or GCC with >=C++20 support
* Make
* SVML (optional)
* Premake (optional, fallback Makefile that does not compile tests exitsts)

## Compiling
```sh
make build  # ARGS=[additional args] (only when using premake)
            #   --use-gcc   use gcc instead of clang
            #   --with-svml use SVML routines as defaults
```

# Used Libraries
* [T-SIMD](http://www.ti.uni-bielefeld.de/html/people/moeller/tsimd_warpingsimd.html)
* [GLFW](https://github.com/glfw/glfw)
* [ImGui](https://github.com/ocornut/imgui)
* [fmt](https://github.com/fmtlib/fmt)
* [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp)
* [Vk-Bootsrap](https://github.com/charles-lunarg/vk-bootstrap)
* [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
* [stb](https://github.com/nothings/stb)
* [jevents (pmu-tools)](https://github.com/andikleen/pmu-tools/tree/master/jevents)
* [vector-class-library](https://github.com/vectorclass/version2)
* [tiny-obj-loader](https://github.com/tinyobjloader/tinyobjloader)
