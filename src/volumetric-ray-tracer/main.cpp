#include <cmath>
#include <cstdlib>
#include <fmt/core.h>
#include <thread>
#include <vk-renderer/vk-renderer.h>
#include <include/error_fmt.h>
#include <sys/wait.h>
#include <sched.h>
#include <getopt.h>
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

struct cmd_args_t
{
    u64 w = -1, h = -1;
    u64 grid_dim = 4;
    char *outfile = nullptr;
    char *infile = nullptr;
    bool use_grid = false;
    cmd_args_t(i32 argc, char **argv)
    {
        static struct option opts[] = {
            { "grid", optional_argument, NULL, 'g' },
            { "file", required_argument, NULL, 'f' },
            { "output", required_argument, NULL, 'o' },
            { "width", required_argument, NULL, 'w' },
            { "height", required_argument, NULL, 'h' }
        };
        i32 lidx;
        while (1)
        {
            char c = getopt_long(argc, argv, "w:o:f:g:h:", opts, &lidx);
            if (c == -1)
                break;
            switch(c)
            {
                case 'g':
                    this->use_grid = true;
                    if (optarg != NULL)
                    {
                        this->grid_dim = strtoul(optarg, NULL, 10);
                    }
                    break;
                case 'f':
                    this->infile = optarg;
                    break;
                case 'o':
                    this->outfile = optarg;
                    break;
                case 'w':
                    this->w = strtoul(optarg, NULL, 10);
                    if (this->h == (u64)-1) this->h = this->w;
                    break;
                case 'h':
                    this->h = strtoul(optarg, NULL, 10);
                    if (this->w == (u64)-1) this->w = this->h;
                    break;
            }
        }
        if (this->w == (u64)-1) this->w = 256;
        if (this->h == (u64)-1) this->h = 256;
    }
};

i32 main(i32 argc, char **argv)
{
    cmd_args_t cmd(argc, argv);
    std::vector<gaussian_t> _gaussians;
    if (cmd.use_grid)
    {
        u8 grid_dim = cmd.grid_dim;
        for (u8 i = 0; i < grid_dim; ++i)
            for (u8 j = 0; j < grid_dim; ++j)
                _gaussians.push_back(gaussian_t{
                        .albedo{ 1.f - (i * grid_dim + j)/(f32)(grid_dim*grid_dim), 0.f, 0.f + (i * grid_dim + j)/(f32)(grid_dim*grid_dim), 1.f },
                        .mu{ -1.f + 1.f/grid_dim + i * 1.f/(grid_dim/2.f), -1.f + 1.f/grid_dim + j * 1.f/(grid_dim/2.f), 1.f },
                        .sigma = 1.f/(2 * grid_dim),
                        .magnitude = 3.f
                        });
    }
    else
    {
        _gaussians = { gaussian_t{ .albedo{ 0.f, 1.f, 0.f, 1.f }, .mu{ .3f, .3f, .5f }, .sigma = 0.1f, .magnitude = 2.f }, gaussian_t{ .albedo{ 0.f, 0.f, 1.f, 1.f }, .mu{ -.3f, -.3f, 1.f }, .sigma = 0.4f, .magnitude = .7f }, gaussian_t{ .albedo{ 1.f, 0.f, 0.f, 1.f }, .mu{ 0.f, 0.f, 2.f }, .sigma = .75f, .magnitude = 1.f } };
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
    bool use_simd_transmittance = true;
    bool use_simd_pixels = false;
    bool use_tiling = true;

    u64 width = cmd.w, height = cmd.h;
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
        ImGui::Checkbox("use tiling", &use_tiling);
        ImGui::Checkbox("use simd innermost", &use_simd_transmittance);
        ImGui::Checkbox("use simd pixels", &use_simd_pixels);
        ImGui::End();
    };
    bool running = true;
    std::thread render_thread([&](){ renderer.run(running); });

    u32 *image = (u32*)simd_aligned_malloc(SIMD_BYTES, sizeof(u32) * width * height);
    u32 *bg_image = (u32*)simd_aligned_malloc(SIMD_BYTES, sizeof(u32) * width * height);

    bool w = false, h = false;
    for (size_t x = 0; x < width; ++x)
    {
        if (!(x % 16)) w = !w;
        for (size_t y = 0; y < height; ++y)
        {
            if (!(y % 16)) h = !h;
            bg_image[y * width + x] = (w ^ h) ? 0xFFAAAAAA : 0xFF555555;
        }
    }

    f32 *xs, *ys;
    GENERATE_PROJECTION_PLANE(xs, ys, width, height);
    struct timespec start, end;

    while (running)
    {
        gaussians.gaussians = staging_gaussians;
        gaussians.gaussians_broadcast->load_gaussians(staging_gaussians);
        tiles_t tiles = tile_gaussians(.5f, .5f, staging_gaussians, glm::mat4(1));
        if (use_spline_approx) _erf = approx::spline_erf;
        else if (use_mirror_approx) _erf = approx::spline_erf_mirror;
        else if (use_taylor_approx) _erf = approx::taylor_erf;
        else if (use_abramowitz_approx) _erf = approx::abramowitz_stegun_erf;
        else _erf = std::erf;
        if (use_fast_exp) _exp = approx::fast_exp;
        else _exp = std::exp;
        if (use_simd_transmittance) _transmittance = simd_transmittance;
        else { _transmittance = transmittance; }

        //auto start_time = std::chrono::system_clock::now();
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
        bool res = false;
        if (use_tiling)
        {
            if (use_simd_pixels) res = simd_render_image(width, height, image, (i32*)bg_image, xs, ys, tiles, running);
            else res = render_image(width, height, image, bg_image, xs, ys, tiles, running);
        }
        else
        {
            if (use_simd_pixels) res = simd_render_image(width, height, image, (i32*)bg_image, xs, ys, gaussians, running);
            else res = render_image(width, height, image, bg_image, xs, ys, gaussians, running);
        }
        //auto end_time = std::chrono::system_clock::now();
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
        draw_time = simd::timeSpecDiffNsec(end, start)/1000000.f;
        if (res) break;

        if (cmd.outfile != nullptr)
            stbi_write_png(cmd.outfile, width, height, 4, image, width * 4);
        renderer.stage_image(image, width, height);
    }

    render_thread.join();
    simd_aligned_free(image);
    simd_aligned_free(bg_image);
    simd_aligned_free(xs);
    simd_aligned_free(ys);

    return EXIT_SUCCESS;
}
