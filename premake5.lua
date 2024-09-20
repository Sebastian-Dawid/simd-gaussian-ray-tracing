ARCH = os.getenv("ARCH")
if ARCH == nil then
    ARCH = "native"
end
local svml_path = os.findlib("svml")
SVML_AVAILABLE = svml_path~=nil

workspace "ba-thesis"
    newoption {
        trigger = "use-gcc",
        description = "compile using the gcc toolset"
    }
    newoption {
        trigger = "with-hip",
        description = "compile gpu version of the software"
    }
    newoption {
        trigger = "with-svml",
        description = "link intels svml library"
    }
    newoption {
        trigger = "save-temps",
        description = "compile with the -save-temps flag"
    }
    filter { "options:use-gcc" }
        toolset "gcc"
    filter { "not options:use-gcc" }
        toolset "clang"
    filter {}

    filter { "options:save-temps" }
        buildoptions { "-save-temps=obj" }
    filter {}

    cppdialect "c++20"
    configurations { "debug", "release" }
    location "build"

    if SVML_AVAILABLE then
        filter "options:with-svml"
            defines { "WITH_SVML" }
    end

    filter "configurations:debug"
        defines { "DEBUG" }
        symbols "On"
        buildoptions { "-save-temps=obj" }

    filter "configurations:release"
        defines { "NDEBUG" }
        optimize "On"
        symbols "On"

    project "imgui"
        kind "StaticLib"
        language "C++"
        targetdir "build/lib"

        includedirs { "./src/external/imgui", "./src/external/imgui/backend" }
        files { "./src/external/imgui/**.cpp" }

    project "jevents"
        kind "StaticLib"
        language "C"
        targetdir "build/lib"

        files { "./src/jevents/*.c" }

    project "vk-renderer"
        kind "StaticLib"
        language "C++"
        targetdir "build/lib"
        buildoptions { "-Wall", "-Wextra", "-Werror" }
        defines { "VULKAN_HPP_NO_EXCEPTIONS" }

        filter "files:src/vk-renderer/vk-mem-alloc/vk_mem_alloc.cpp"
            buildoptions { "-w" }
        filter {}
        includedirs { "./src", "./src/external/imgui/" }
        files { "./src/vk-renderer/**.cpp" }

    project "vrt"
        kind "SharedLib"
        language "C++"
        targetdir "build/lib"
        buildoptions { "-Wall", "-Wextra", "-Werror", "-march="..ARCH, "-fverbose-asm", "-ffast-math" }
        defines { "INCLUDE_IMGUI" }

        includedirs { "./src", "./src/external/imgui/" }
        files { "./src/vrt/*.cpp" }

    project "volumetric-ray-tracer"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin"
        buildoptions { "-Wall", "-Wextra", "-Werror", "-march="..ARCH, "-fverbose-asm", "-ffast-math" }
        defines { "INCLUDE_IMGUI" }

        if SVML_AVAILABLE then
            filter { "options:with-svml" }
                libdirs { svml_path }
                links { "svml" }
            filter {}
        end
        includedirs { "./src", "./src/external/imgui/" }
        files { "./src/volumetric-ray-tracer/*.cpp" }
        links { "fmt", "glfw", "vulkan", "vk-renderer", "imgui", "vrt" }

    project "hip-volumetric-ray-tracer"
        kind "None"
        filter { "options:with-hip" }
            kind "Utility" -- I can't force premake to use hipcc and Makefile projects are only supported for VS according to the docs
        filter {}
        targetdir "build/bin"
        dependson { "imgui", "vk-renderer" }

        filter "files:**.hip"
            buildmessage 'BUILDING %{file.name}'
            buildcommands { 'mkdir -p bin', 'hipcc -std=c++20 -I../src -I../src/external/imgui -o "bin/%{file.name:match("(.+)%..+$")}" "%{file.relpath}" -lfmt -lglfw -lvulkan "-L./lib" -lvk-renderer -limgui -DGRID -DWITH_RENDERER' }
            buildoutputs { 'bin/%{file.name:match("(.+)%..+$")}' }
        filter {}

        files { "./src/volumetric-ray-tracer/rocm-rt.hip" }

    if SVML_AVAILABLE then
        project "cycles-test"
            kind "ConsoleApp"
            language "C++"
            targetdir "build/bin"
            buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-fverbose-asm", "-ffast-math", "-fno-unroll-loops" }
            defines { "WITH_SVML" }

            libdirs { svml_path }
            includedirs { "./src", "./src/jevents" }
            files { "./src/volumetric-ray-tracer/tests/approx_cycles.cpp" }
            links { "fmt", "jevents", "vrt", "svml" }

        project "accuracy-test"
            kind "ConsoleApp"
            language "C++"
            targetdir "build/bin"
            buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-fverbose-asm", "-ffast-math" }
            defines { "WITH_SVML" }

            libdirs { svml_path }
            includedirs { "./src" }
            files { "./src/volumetric-ray-tracer/tests/accuracy.cpp" }
            links { "fmt", "vrt", "svml" }
    end

    project "transmittance-test"
        kind "ConsoleApp"
        language "C++"
        targetdir "build/bin"
        buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-fverbose-asm", "-ffast-math" }

        if SVML_AVAILABLE then
            filter { "options:with-svml" }
                libdirs { svml_path }
                links { "svml" }
            filter {}
        end
        includedirs { "./src" }
        files { "./src/volumetric-ray-tracer/tests/transmittance.cpp" }
        links { "fmt", "vrt" }

    if SVML_AVAILABLE then
        project "img-error-test"
            kind "ConsoleApp"
            language "C++"
            targetdir "build/bin"
            buildoptions { "-Wall", "-Wextra", "-march="..ARCH, "-fverbose-asm", "-ffast-math" }
            defines { "WITH_SVML" }

            libdirs { svml_path }
            includedirs { "./src" }
            files { "./src/volumetric-ray-tracer/tests/img-error.cpp" }
            links { "fmt", "svml", "vrt" }
    end
