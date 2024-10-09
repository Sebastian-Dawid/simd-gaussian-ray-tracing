const std = @import("std");

const flags = [_][]const u8{
    "-DFMT_HEADER_ONLY",
    "-DVULKAN_HPP_NO_EXCEPTIONS",
    "-DINCLUDE_IMGUI",
    "-O3",
    "-g",
    "-march=native",
    "-ffast-math",
    "-std=c++20",
};
const includes = [_][]const u8{ "src", "src/external/imgui" };
const sys_libs = [_][]const u8{ "fmt", "vulkan", "glfw" };

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const imgui = b.addStaticLibrary(.{
        .name = "imgui",
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    imgui.addCSourceFiles(.{
        .files = &.{
            "src/external/imgui/imgui.cpp",
            "src/external/imgui/imgui_demo.cpp",
            "src/external/imgui/imgui_draw.cpp",
            "src/external/imgui/imgui_tables.cpp",
            "src/external/imgui/imgui_widgets.cpp",
            "src/external/imgui/backend/imgui_impl_glfw.cpp",
            "src/external/imgui/backend/imgui_impl_vulkan.cpp",
        },
        .flags = &flags,
    });

    const vk_renderer = b.addStaticLibrary(.{
        .name = "vk-renderer",
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    vk_renderer.addCSourceFiles(.{
        .files = &.{
            "src/vk-renderer/vk-renderer.cpp",
            "src/vk-renderer/VkBootstrap/VkBootstrap.cpp",
            "src/vk-renderer/vk-mem-alloc/vk_mem_alloc.cpp",
        },
        .flags = &flags,
    });

    const vrt = b.addSharedLibrary(.{
        .name = "vrt",
        .target = target,
        .optimize = optimize,
        .pic = true,
        .link_libc = true,
    });
    vrt.addCSourceFiles(.{
        .files = &.{
            "src/vrt/rt.cpp",
            "src/vrt/types.cpp",
            "src/vrt/approx.cpp",
            "src/vrt/camera.cpp",
            "src/vrt/gaussians-from-file.cpp",
            "src/vrt/thread-pool.cpp",
        },
        .flags = &flags,
    });

    const exe = b.addExecutable(.{
        .name = "volumetric-ray-tracer",
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    exe.addCSourceFile(.{
        .file = b.path("src/volumetric-ray-tracer/main.cpp"),
        .flags = &flags,
    });
    exe.linkLibCpp();

    inline for (sys_libs) |lib| exe.linkSystemLibrary(lib);

    const libs = [_]@TypeOf(vrt){ imgui, vk_renderer, vrt };
    for (libs) |lib| {
        inline for (sys_libs) |l| lib.linkSystemLibrary(l);
        inline for (includes) |inc| lib.addIncludePath(b.path(inc));
        lib.linkLibCpp();
        b.installArtifact(lib);
        exe.linkLibrary(lib);
    }

    inline for (includes) |inc| exe.addIncludePath(b.path(inc));
    b.installArtifact(exe);
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
