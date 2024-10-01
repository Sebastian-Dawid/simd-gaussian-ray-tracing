add_library(jevents
    json.c jsmn.c jevents.c resolve.c cache.c cpustr.c rawevent.c perf-iter.c interrupts.c
    rdpmc.c measure.c perf_event_open.c session.c perf-aux.c csv.c print.c
)
target_include_directories(jevents INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(jevents PUBLIC compiler_flags)
target_compile_options(jevents PRIVATE -w)
