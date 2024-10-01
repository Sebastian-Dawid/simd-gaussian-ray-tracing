cmake_minimum_required(VERSION 3.15)

project(GaussianRayTracing VERSION 1.0)

add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_20)

target_compile_options(compiler_flags INTERFACE "-O3;-Wall;-Wextra;-march=native;-fverbose-asm;-ffast-math")
target_link_libraries(compiler_flags INTERFACE m stdc++)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

option(WITH_SVML "Use SVML for the default implementations." OFF)
option(SYMBOLS "" OFF)

if(WITH_SVML)
    target_compile_definitions(compiler_flags INTERFACE WITH_SVML)
endif()

add_subdirectory(src/vrt)
add_subdirectory(src/vk-renderer)
add_subdirectory(src/jevents)
add_subdirectory(src/external/imgui)

add_executable(volumetric-ray-tracer src/volumetric-ray-tracer/main.cpp)
target_compile_definitions(volumetric-ray-tracer PUBLIC INCLUDE_IMGUI)
target_link_libraries(volumetric-ray-tracer PUBLIC compiler_flags fmt glfw vulkan vk-renderer imgui vrt)

target_include_directories(volumetric-ray-tracer PUBLIC "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/src/external/imgui")

if(WITH_SVML)
    target_link_libraries(volumetric-ray-tracer PUBLIC svml)
endif()
