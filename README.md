# SIMD Guassian Ray Tracing
<p>
<img src="./images/tea.gif" width=256 height=256 />
<img src="./images/cube.gif" width=256 height=256 />
</p>

**Figure 1** - Example Images

The project is split up into three main components:
1. The `vrt` ray tracing library
2. The `vk-renderer` library for displaying images
3. The `volumetric-ray-tracer` example application

All components require `make` and `gcc` version 14 or `clang` version 18. Earlier versions might work, but I have not tested them.

## Compiling
The external dependencies needed for compilation are:
* GLFW (`vk-renderer`, `libglfw3-dev` on Debian)
* FMT (all, `libfmt-dev` on Debian)
* GLM (`vrt`, `vk-renderer`, `libglm-dev` on Debian)
* Vulkan (`vk-renderer`, `libvulkan-dev`, `vulkan-validationlayers` and `spirv-tools` on Debian)
* SVML (`vrt`, optional, distributed through Intel's compiler)

On Debian compilation might fail using `gcc`, therefore using `clang` is recommended.

[Premake](https://premake.github.io/) can be used for compilation, but there is a fallback Makefile and CMake Project in
case premake is not available. A list of all targets available in the Makefile can be accessed using `make help` or just `make`.

The Makefile variables `USE_CMAKE` and `USE_MAKEFILE` can be used to override premake usage in case it is available on the
system but CMake or the Makefile are preferred.

Note that the compiler has to be set using the `CXX` environment variable if CMake or the fallback Makefile are used. For premake `clang` is the default.

## Usage
```
$ ./build/bin/volumetric-ray-tracer --help     # List all options
$ ./build/bin/volumetric-ray-tracer -w <width> # Sets the width of the image in pixels to <width> (Default is 256)
$ ./build/bin/volumetric-ray-tracer -f <path>  # Load and interpret the file at <path> as a set of Gaussians
$ ./build/bin/volumetric-ray-tracer -g <dim=4> # Render a <dim>x<dim> grid of Gaussians. (Default if no file is specified)
$ ./build/bin/volumetric-ray-tracer -q         # Render without displaying image
$ ./build/bin/volumetric-ray-tracer -o <path>  # Write rendered image to <path>
$ ./build/bin/volumetric-ray-tracer -t <no>    # Set the number of threads to use to <no>
$ ./build/bin/volumetric-ray-tracer -m <mode>  # Set the initial rendering mode to <mode>
```

Starting the example program without the `-q` flag will display the rendered image along with two floating widows that
enable modifying the scene and changing the rendering mode in use.
Note that the checkboxes regarding the mode of SIMD parallelization override each other from top to bottom. Meaning that
if the bottom most checkbox is enabled the ones above it have no effect until it is unchecked.

This repository contains five example objects located in the `test-objects` directory.

# Used Libraries
* [T-SIMD](http://www.ti.uni-bielefeld.de/html/people/moeller/tsimd_warpingsimd.html)
* [GLFW](https://github.com/glfw/glfw)
* [GLM](https://github.com/g-truc/glm)
* [ImGui](https://github.com/ocornut/imgui)
* [fmt](https://github.com/fmtlib/fmt)
* [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp)
* [Vk-Bootstrap](https://github.com/charles-lunarg/vk-bootstrap)
* [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
* [stb](https://github.com/nothings/stb)
* [jevents (pmu-tools)](https://github.com/andikleen/pmu-tools/tree/master/jevents)
* [vector-class-library](https://github.com/vectorclass/version2)
* [tiny-obj-loader](https://github.com/tinyobjloader/tinyobjloader)
* [SVML](https://www.intel.com/content/www/us/en/docs/cpp-compiler/developer-guide-reference/2021-8/intrinsics-for-short-vector-math-library-ops.html)
