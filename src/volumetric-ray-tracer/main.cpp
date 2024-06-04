#include <cmath>
#include <cstdlib>
#include <fmt/core.h>
#include <thread>
#include <vk-renderer/vk-renderer.h>
#include <include/error_fmt.h>
#include <sys/wait.h>
#include <sched.h>

#include <malloc.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wuninitialized"

#include <include/tsimd_sh.H>
#include <include/TimeMeasurement.H>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <include/stb_image_write.h>

#pragma GCC diagnostic pop

#include "types.h"
#include "approx.h"
#include "rt.h"

int main(i32 argc, char **argv)
{
    u64 width = 256, height = 256;
    if (argc >= 3)
    {
        width = strtoul(argv[1], NULL, 10);
        if (width == 0)
        {
            fmt::println(stderr, "[ {} ]\tInvaid width set! Setting width to 256.", ERROR_FMT("ERROR"));
            width = 256;
        }
        height = strtoul(argv[2], NULL, 10);
        if (height == 0)
        {
            fmt::println(stderr, "[ {} ]\tInvaid height set! Setting height to 256.", ERROR_FMT("ERROR"));
            height = 256;
        }
    }
    u64 new_width = width, new_height = height;
    
    std::vector<char*> args(argc);
    std::memcpy(args.data(), argv, sizeof(*argv) * argc);
    bool grid = false;
    u64 idx = 0;
    u64 filename_idx = -1;
    u64 grid_width_idx = -1;
    for (char *s : args)
    {
        if (std::strncmp(s, "--grid", 7) == 0)
        {
            grid = true;
            if ((u64)argc > idx + 1 && isdigit(argv[idx + 1][0]))
                grid_width_idx = idx + 1;
        }
        if (std::strncmp(s, "-o", 3) == 0) filename_idx = idx + 1;
        ++idx;
    }

    std::vector<gaussian_t> _gaussians;
    if (grid)
    {
        constexpr u8 base_dim = 4;
        u8 grid_dim = (grid_width_idx != (u64)-1) ? strtoul(argv[grid_width_idx], NULL, 10) : base_dim;
        for (u8 i = 0; i < grid_dim; ++i)
            for (u8 j = 0; j < grid_dim; ++j)
                _gaussians.push_back(gaussian_t{
                        .albedo{ 1.f - (i * grid_dim + j)/(f32)(grid_dim*grid_dim), 0.f, 0.f + (i * grid_dim + j)/(f32)(grid_dim*grid_dim), 1.f },
                        .mu{ -1.f + 1.f/grid_dim + i * 1.f/(grid_dim/2.f), -1.f + 1.f/grid_dim + j * 1.f/(grid_dim/2.f), 0.f },
                        .sigma = 1.f/(2 * grid_dim),
                        .magnitude = 3.f
                        });
    }
    else
    {
        _gaussians = { gaussian_t{ .albedo{ 0.f, 1.f, 0.f, .1f }, .mu{ .3f, .3f, .5f }, .sigma = 0.1f, .magnitude = 2.f }, gaussian_t{ .albedo{ 0.f, 0.f, 1.f, .7f }, .mu{ -.3f, -.3f, 0.f }, .sigma = 0.4f, .magnitude = .7f }, gaussian_t{ .albedo{ 1.f, 0.f, 0.f, 1.f }, .mu{ 0.f, 0.f, 2.f }, .sigma = .75f, .magnitude = 1.f } };
    }
    std::vector<gaussian_t> staging_gaussians = _gaussians;
    gaussians_t gaussians{ .gaussians = _gaussians, .gaussians_broadcast = gaussian_vec_t::from_gaussians(_gaussians) };
    
    renderer_t renderer;// = new renderer_t();
    f32 draw_time = 0.f;
    bool use_spline_approx = false;
    bool use_mirror_approx = false;
    bool use_taylor_approx = false;
    bool use_abramowitz_approx = false;
    bool use_fast_exp = false;
    bool use_simd_transmittance = false;
    bool use_simd_pixels = false;

    if (!renderer.init(width, height, "Test")) return EXIT_FAILURE;
    renderer.custom_imgui = [&](){
        ImGui::Begin("Gaussians");
        for (gaussian_t &g : staging_gaussians) g.imgui_controls();
        ImGui::End();
        ImGui::Begin("Debug");
        ImGui::Text("Draw Time: %f ms", draw_time);
        ImGui::Checkbox("erf spline", &use_spline_approx);
        ImGui::Checkbox("erf mirror", &use_mirror_approx);
        ImGui::Checkbox("erf taylor", &use_taylor_approx);
        ImGui::Checkbox("erf abramowitz", &use_abramowitz_approx);
        ImGui::Checkbox("exp fast", &use_fast_exp);
        ImGui::Checkbox("use simd innermost", &use_simd_transmittance);
        ImGui::Checkbox("use simd pixels", &use_simd_pixels);
        ImGui::End();
    };
    bool running = true;
    std::thread render_thread([&](){ renderer.run(running); });

    u32 *image = (u32*)simd_aligned_malloc(SIMD_BYTES, sizeof(u32) * width * height);
    f32 *xs, *ys;
    GENERATE_PROJECTION_PLANE(xs, ys, width, height);

    while (running)
    {
        if (new_width != width || new_height != height)
        {
            width = new_width;
            height = new_height;
            image = (u32*)std::realloc(image, width * height * sizeof(u32));
        }
        gaussians.gaussians = staging_gaussians;
        gaussians.gaussians_broadcast.load_gaussians(staging_gaussians);
        if (use_spline_approx) _erf = spline_erf;
        else if (use_mirror_approx) _erf = spline_erf_mirror;
        else if (use_taylor_approx) _erf = taylor_erf;
        else if (use_abramowitz_approx) _erf = abramowitz_stegun_erf;
        else _erf = std::erf;
        if (use_fast_exp) _exp = fast_exp;
        else _exp = std::exp;
        if (use_simd_transmittance) _transmittance = simd_transmittance;
        else { _transmittance = transmittance; }

        auto start_time = std::chrono::system_clock::now();
        bool res = false;
        if (use_simd_pixels) res = simd_render_image(width, height, image, xs, ys, gaussians, running);
        else res = render_image(width, height, image, xs, ys, gaussians, running);
        auto end_time = std::chrono::system_clock::now();
        draw_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        if (res) break;

        if (filename_idx != (u64)-1)
            stbi_write_png(argv[filename_idx], width, height, 4, image, width * 4);
        renderer.stage_image(image, width, height);
    }

    render_thread.join();
    simd_aligned_free(image);
    simd_aligned_free(xs);
    simd_aligned_free(ys);

    malloc_stats();

    return EXIT_SUCCESS;
}
