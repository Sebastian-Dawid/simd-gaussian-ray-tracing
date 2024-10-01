add_library(imgui
    imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp
    backend/imgui_impl_glfw.cpp backend/imgui_impl_vulkan.cpp
)
target_include_directories(imgui
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/backend>
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(imgui compiler_flags)
