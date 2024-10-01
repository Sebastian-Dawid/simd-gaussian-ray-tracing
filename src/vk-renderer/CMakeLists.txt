add_library(vk-renderer
    vk-renderer.cpp VkBootstrap/VkBootstrap.cpp vk-mem-alloc/vk_mem_alloc.cpp
)
target_include_directories(vk-renderer
    INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/external/imgui>
    PUBLIC ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/external/imgui
)
target_compile_definitions(vk-renderer PUBLIC "VULKAN_HPP_NO_EXCEPTIONS")
target_link_libraries(vk-renderer PUBLIC compiler_flags)
target_compile_options(vk-renderer PRIVATE -w)
