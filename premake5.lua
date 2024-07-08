ARCH = os.getenv("ARCH")
if ARCH == nil then
    ARCH = "native"
end
NVIDIA = os.getenv("NVIDIA")

workspace "ba-thesis"
    newoption {
        trigger = "with-clang",
        description = "compile using the clang toolset"
    }
    newoption {
        trigger = "with-rocm",
        description = "compile gpu version of the software"
    }
    filter { "options:with-clang" }
        toolset "clang"
    filter {}
    cppdialect "c++20"
    configurations { "debug", "release" }
    location "build"

    if NVIDIA ~= nil then
        defines { "NVIDIA" }
    end

    filter "configurations:debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:release"
        defines { "NDEBUG" }
        optimize "On"
        symbols "On"

    project "imgui"
        kind "StaticLib"
        language "C++"
        targetdir "build/lib/%{cfg.buildcfg}"

        includedirs { "./src/external/imgui", "./src/external/imgui/backend" }
        files { "./src/external/imgui/**.cpp" }

    project "jevents"
        kind "StaticLib"
        language "C"
        targetdir "build/lib/%{cfg.buildcfg}"

        files { "./src/jevents/*.c" }

    project "vk-renderer"
        kind "StaticLib"
        language "C++"
        targetdir "build/lib/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-Werror" }
        defines { "VULKAN_HPP_NO_EXCEPTIONS" }

        filter "files:src/vk-renderer/vk-mem-alloc/vk_mem_alloc.cpp"
            buildoptions { "-w" }
        filter {}
        includedirs { "./src", "./src/external/imgui/" }
        files { "./src/vk-renderer/**.cpp" }

    project "volumetric-ray-tracer"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-Werror", "-march="..ARCH, "-save-temps=obj", "-masm=intel", "-fverbose-asm", "-ffast-math" }
        defines { "INCLUDE_IMGUI" }

        libdirs { "/opt/intel/oneapi/compiler/latest/lib" }
        includedirs { "./src", "./src/external/imgui/" }
        files { "./src/volumetric-ray-tracer/*.cpp" }
        links { "fmt", "glfw", "vulkan", "vk-renderer", "imgui", "rt", "svml" }

    project "hip-volumetric-ray-tracer"
        kind "None"
        filter { "options:with-rocm" }
            kind "Utility" -- I can't force premake to use hipcc and Makefile projects are only supported for VS according to the docs
        filter {}
        targetdir "build/bin/%{cfg.buildcfg}"
        dependson { "imgui", "vk-renderer" }

        filter "files:**.hip"
            buildmessage 'BUILDING %{file.name}'
            if NVIDIA == nil then
                buildcommands { 'mkdir -p bin/%{cfg.buildcfg}', 'hipcc -std=c++20 -I../src -I../src/external/imgui -o "bin/%{cfg.buildcfg}/%{file.name:match("(.+)%..+$")}" "%{file.relpath}" -lfmt -lglfw -lvulkan "-L./lib/%{cfg.buildcfg}" -lvk-renderer -limgui -DGRID -DWITH_RENDERER' }
            else
                buildcommands { 'mkdir -p bin/%{cfg.buildcfg}', 'hipcc -std=c++20 -I../src -o "bin/%{cfg.buildcfg}/%{file.name:match("(.+)%..+$")}" "%{file.relpath}" -DGRID' }
            end
            buildoutputs { 'bin/%{cfg.buildcfg}/%{file.name:match("(.+)%..+$")}' }
        filter {}

        files { "./src/volumetric-ray-tracer/rocm-rt.hip" }

    project "cycles-test"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-masm=intel", "-fverbose-asm", "-ffast-math" }

        includedirs { "./src", "./src/jevents" }
        files { "./src/volumetric-ray-tracer/tests/approx_cycles.cpp", "./src/volumetric-ray-tracer/approx.cpp" }
        links { "fmt", "jevents" }

    project "accuracy-test"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-masm=intel", "-fverbose-asm", "-ffast-math" }

        includedirs { "./src" }
        files { "./src/volumetric-ray-tracer/tests/accuracy.cpp", "./src/volumetric-ray-tracer/approx.cpp" }
        links { "fmt" }

    project "transmittance-test"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-masm=intel", "-fverbose-asm", "-ffast-math" }

        includedirs { "./src" }
        files { "./src/volumetric-ray-tracer/tests/transmittance.cpp", "./src/volumetric-ray-tracer/rt.cpp", "./src/volumetric-ray-tracer/approx.cpp" }
        links { "fmt" }

    project "timing-test"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-masm=intel", "-fverbose-asm", "-ffast-math" }

        includedirs { "./src" }
        files { "./src/volumetric-ray-tracer/tests/timing.cpp", "./src/volumetric-ray-tracer/rt.cpp", "./src/volumetric-ray-tracer/approx.cpp" }
        links { "fmt" }

    project "image-timing-test"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-masm=intel", "-fverbose-asm", "-ffast-math" }

        includedirs { "./src" }
        files { "./src/volumetric-ray-tracer/tests/image-timing.cpp", "./src/volumetric-ray-tracer/rt.cpp", "./src/volumetric-ray-tracer/approx.cpp" }
        links { "fmt" }

    project "perf-test"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-masm=intel", "-fverbose-asm", "-ffast-math" }

        libdirs { "/opt/intel/oneapi/compiler/latest/lib" }
        includedirs { "./src" }
        files { "./src/volumetric-ray-tracer/tests/perf-test.cpp", "./src/volumetric-ray-tracer/rt.cpp", "./src/volumetric-ray-tracer/approx.cpp" }
        links { "fmt", "svml" }

    project "svml-test"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        -- compiling fogs avx2 exp implementation uses inline assemby that breaks when using -masm=intel 
        buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj",  "-fverbose-asm", "-ffast-math" }

        libdirs { "/opt/intel/oneapi/compiler/latest/lib" }
        includedirs { "./src" }
        files { "./src/volumetric-ray-tracer/tests/svml-test.cpp", "./src/volumetric-ray-tracer/approx.cpp" }
        links { "fmt", "svml" }

    project "img-error-test"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        -- see svml-test on why -masm=intel is missing
        buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-fverbose-asm", "-ffast-math" }

        libdirs { "/opt/intel/oneapi/compiler/latest/lib" }
        includedirs { "./src" }
        files { "./src/volumetric-ray-tracer/tests/img-error.cpp", "./src/volumetric-ray-tracer/approx.cpp", "./src/volumetric-ray-tracer/rt.cpp" }
        links { "fmt", "svml" }

    project "vrt-unit-tests"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-ffast-math" }

        libdirs { "/opt/intel/oneapi/compiler/latest/lib" }
        includedirs { "./src" }
        files { "./src/volumetric-ray-tracer/unit-tests/**", "./src/volumetric-ray-tracer/approx.cpp", "./src/volumetric-ray-tracer/rt.cpp" }
        links { "fmt", "svml", "gtest" }
