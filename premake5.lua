ARCH = os.getenv("ARCH")
if ARCH == nil then
    ARCH = "native"
end
NVIDIA = os.getenv("NVIDIA")
local svml_path = os.findlib("vcl")
SVML_AVAILABLE = svml_path~=nil

workspace "ba-thesis"
    newoption {
        trigger = "use-gcc",
        description = "compile using the gcc toolset"
    }
    newoption {
        trigger = "with-rocm",
        description = "compile gpu version of the software"
    }
    newoption {
        trigger = "with-svml",
        description = "link intels svml library"
    }
    filter { "options:with-gcc" }
        toolset "gcc"
    filter { "not options:with-gcc" }
        toolset "clang"
    filter {}

    cppdialect "c++20"
    configurations { "debug", "release" }
    location "build"

    if NVIDIA ~= nil then
        defines { "NVIDIA" }
    end

    if SVML_AVAILABLE then
        filter "options:with-svml"
            defines { "WITH_SVML" }
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
        includedirs { "./src", "./src/external/imgui/", "./src/external/fmt/include/" }
        files { "./src/vk-renderer/**.cpp" }

    project "vrt"
        kind "SharedLib"
        language "C++"
        targetdir "build/lib/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-Werror", "-march="..ARCH, "-save-temps=obj", "-fverbose-asm", "-ffast-math" }
        defines { "INCLUDE_IMGUI" }

        includedirs { "./src", "./src/external/imgui/", "./src/external/fmt/include/" }
        files { "./src/vrt/*.cpp" }

    project "volumetric-ray-tracer"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-Werror", "-march="..ARCH, "-save-temps=obj", "-fverbose-asm", "-ffast-math" }
        defines { "INCLUDE_IMGUI" }

        if SVML_AVAILABLE then
            filter { "options:with-svml" }
                libdirs { svml_path }
                links { "svml" }
            filter {}
        end
        includedirs { "./src", "./src/external/imgui/", "./src/external/fmt/include/" }
        files { "./src/volumetric-ray-tracer/*.cpp" }
        links { "fmt", "glfw", "vulkan", "vk-renderer", "imgui", "vrt" }

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

    if SVML_AVAILABLE then
        project "cycles-test"
            kind "ConsoleApp"
            language "C++"
            targetdir "build/bin/%{cfg.buildcfg}"
            buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-fverbose-asm", "-ffast-math" }
            defines { "WITH_SVML" }

            libdirs { svml_path }
            includedirs { "./src", "./src/jevents" }
            files { "./src/volumetric-ray-tracer/tests/approx_cycles.cpp" }
            links { "fmt", "jevents", "vrt", "svml" }

        project "accuracy-test"
            kind "ConsoleApp"
            language "C++"
            targetdir "build/bin/%{cfg.buildcfg}"
            buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-fverbose-asm", "-ffast-math" }
            defines { "WITH_SVML" }

            libdirs { svml_path }
            includedirs { "./src" }
            files { "./src/volumetric-ray-tracer/tests/accuracy.cpp" }
            links { "fmt", "vrt", "svml" }
    end

    project "transmittance-test"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-fverbose-asm", "-ffast-math" }

        if SVML_AVAILABLE then
            filter { "options:with-svml" }
                libdirs { svml_path }
                links { "svml" }
            filter {}
        end
        includedirs { "./src" }
        files { "./src/volumetric-ray-tracer/tests/transmittance.cpp" }
        links { "fmt", "vrt" }

    project "timing-test"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-fverbose-asm", "-ffast-math" }

        if SVML_AVAILABLE then
            filter { "options:with-svml" }
                libdirs { svml_path }
                links { "svml" }
            filter {}
        end
        includedirs { "./src" }
        files { "./src/volumetric-ray-tracer/tests/timing.cpp" }
        links { "fmt", "vrt" }

    project "image-timing-test"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin/%{cfg.buildcfg}"
        buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-fverbose-asm", "-ffast-math" }

        if SVML_AVAILABLE then
            filter { "options:with-svml" }
                libdirs { svml_path }
                links { "svml" }
            filter {}
        end
        includedirs { "./src" }
        files { "./src/volumetric-ray-tracer/tests/image-timing.cpp" }
        links { "fmt", "vrt" }

    if SVML_AVAILABLE then
        project "perf-test"
            kind "ConsoleApp"
            language "C++"
            targetdir "build/bin/%{cfg.buildcfg}"
            buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-fverbose-asm", "-ffast-math" }
            defines { "WITH_SVML" }

            libdirs { svml_path }
            includedirs { "./src" }
            files { "./src/volumetric-ray-tracer/tests/perf-test.cpp" }
            links { "fmt", "svml", "vrt" }

        project "svml-test"
            kind "ConsoleApp"
            language "C++"
            targetdir "build/bin/%{cfg.buildcfg}"
            -- compiling fogs avx2 exp implementation uses inline assemby that breaks when using -masm=intel 
            buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj",  "-fverbose-asm", "-ffast-math" }
            defines { "WITH_SVML" }

            libdirs { svml_path }
            includedirs { "./src" }
            files { "./src/volumetric-ray-tracer/tests/svml-test.cpp" }
            links { "fmt", "svml", "vrt" }

        project "img-error-test"
            kind "ConsoleApp"
            language "C++"
            targetdir "build/bin/%{cfg.buildcfg}"
            -- see svml-test on why -masm=intel is missing
            buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-save-temps=obj", "-fverbose-asm", "-ffast-math" }
            defines { "WITH_SVML" }

            libdirs { svml_path }
            includedirs { "./src" }
            files { "./src/volumetric-ray-tracer/tests/img-error.cpp" }
            links { "fmt", "svml", "vrt" }
    end
