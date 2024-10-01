add_library(vrt SHARED
    camera.cpp rt.cpp types.cpp approx.cpp gaussians-from-file.cpp thread-pool.cpp
)
target_link_libraries(vrt PUBLIC compiler_flags)
target_include_directories(vrt
    INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/external/imgui>
    PUBLIC ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/external/imgui
)
