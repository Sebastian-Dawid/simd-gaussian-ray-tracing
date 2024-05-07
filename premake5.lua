workspace "ba-thesis"
    toolset "clang"
    cppdialect "c++20"
    configurations { "debug", "release" }
    location "build"

    filter "configurations:debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:release"
        defines { "NDEBUG" }
        optimize "On"

    project "imgui"
        kind "StaticLib"
        language "C++"
        targetdir "build/lib/%{cfg.buildcfg}"

        includedirs { "./src/external/imgui", "./src/external/imgui/backend" }
        files { "./src/external/imgui/**.cpp" }

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
        buildoptions { "-Wall", "-Wextra", "-Werror" }

        includedirs { "./src", "./src/external/imgui/" }
        files { "./src/volumetric-ray-tracer/**.cpp" }
        links { "fmt", "glfw", "vulkan", "vk-renderer", "imgui" }
