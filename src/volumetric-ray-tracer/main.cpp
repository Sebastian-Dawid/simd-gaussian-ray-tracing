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

#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
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
#include "rt.h"
#include "gaussians-from-file.h"
#include "camera.h"

#include <glm/ext.hpp>

struct cmd_args_t
{
    u64 w = -1, h = -1;
    u64 grid_dim = 4;
    char *outfile = nullptr;
    char *infile = nullptr;
    bool use_grid = false;
    bool quiet = false;
    u64 thread_count = 1;
    u64 tiles = 4;
    bool use_tiling = true;
    bool use_simd_transmittance = false;
    bool use_simd_pixels = true;
    cmd_args_t(i32 argc, char **argv)
    {
        static struct option opts[] = {
            { "grid", optional_argument, NULL, 'g' },
            { "file", required_argument, NULL, 'f' },
            { "output", required_argument, NULL, 'o' },
            { "width", required_argument, NULL, 'w' },
            { "height", required_argument, NULL, 'h' },
            { "with-threads", required_argument, NULL, 't' },
            { "quiet", no_argument, NULL, 'q' },
            { "tiles", required_argument, NULL, 'l' },
            { "mode", required_argument, NULL, 'm' }
        };
        i32 lidx;
        while (1)
        {
            char c = getopt_long(argc, argv, "m:qw:o:f:g:h:t:", opts, &lidx);
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
                case 't':
                    //this->use_threads = true;
                    this->thread_count = strtoul(optarg, NULL, 10);
                    break;
                case 'q':
                    this->quiet = true;
                    break;
                case 'l':
                    this->tiles = strtoul(optarg, NULL, 10);
                    break;
                case 'm':
                    u64 mode = strtoul(optarg, NULL, 10);
                    this->use_tiling = false;
                    this->use_simd_transmittance = false;
                    this->use_simd_pixels = false;
                    switch (mode) {
                        case 4: // tiling sequential
                            this->use_tiling = true;
                        case 1: // no tiling sequential
                            break;
                        case 5: // tiling transmittance
                            this->use_tiling = true;
                        case 2: // no tiling transmittance
                            this->use_simd_transmittance = true;
                            break;
                        default:
                        case 6: // tiling pixels
                            this->use_tiling = true;
                        case 3: // no tiling pixels
                            this->use_simd_pixels = true;
                            break;
                    }
                    break;
            }
        }
        if (this->w == (u64)-1) this->w = 256;
        if (this->h == (u64)-1) this->h = 256;
        if (this->use_grid && this->infile != nullptr) this->use_grid = false;
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
    else if (cmd.infile != nullptr)
    {
        _gaussians = read_from_obj(cmd.infile);
    }
    else
    {
        _gaussians = { gaussian_t{ .albedo{ 0.f, 1.f, 0.f, 1.f }, .mu{ .3f, .3f, .5f }, .sigma = 0.1f, .magnitude = 2.f }, gaussian_t{ .albedo{ 0.f, 0.f, 1.f, 1.f }, .mu{ -.3f, -.3f, 1.f }, .sigma = 0.4f, .magnitude = .7f }, gaussian_t{ .albedo{ 1.f, 0.f, 0.f, 1.f }, .mu{ 0.f, 0.f, 2.f }, .sigma = .75f, .magnitude = 10.f } };
    }


    std::vector<gaussian_t> staging_gaussians = _gaussians;
    gaussians_t gaussians{ .gaussians = _gaussians, .gaussians_broadcast = gaussian_vec_t::from_gaussians(_gaussians) };
    
    std::unique_ptr<renderer_t> renderer = (cmd.quiet) ? nullptr : std::make_unique<renderer_t>();
    f32 draw_time = 0.f, tiling_time = 0.f;
    //bool use_spline_approx = false;
    //bool use_mirror_approx = false;
    //bool use_taylor_approx = false;
    //bool use_abramowitz_approx = false;
    //bool use_fast_exp = false;
    bool use_simd_transmittance = cmd.use_simd_transmittance;
    bool use_simd_pixels = cmd.use_simd_pixels;
    bool use_tiling = cmd.use_tiling;

    u64 width = cmd.w, height = cmd.h;
    bool running = true;
    if (renderer != nullptr)
    {
        if (!renderer->init(width, height, "SIMD VRT")) return EXIT_FAILURE;
        renderer->custom_imgui = [&](){
            ImGui::Begin("Gaussians");
            for (gaussian_t &g : staging_gaussians) g.imgui_controls();
            ImGui::End();
            ImGui::Begin("Debug");
            ImGui::Text("Tiling Time: %f ms", tiling_time);
            ImGui::Text("Draw Time: %f ms", draw_time);
            ImGui::Text("Frame Time: %f ms", draw_time + tiling_time);
            ImGui::Text("FPS: %f", 1000.f / (draw_time + tiling_time));
            //ImGui::Checkbox("erf spline", &use_spline_approx);
            //ImGui::Checkbox("erf mirror", &use_mirror_approx);
            //ImGui::Checkbox("erf taylor", &use_taylor_approx);
            //ImGui::Checkbox("erf abramowitz", &use_abramowitz_approx);
            //ImGui::Checkbox("exp fast", &use_fast_exp);
            ImGui::Checkbox("use tiling", &use_tiling);
            ImGui::Checkbox("use simd innermost", &use_simd_transmittance);
            ImGui::Checkbox("use simd pixels", &use_simd_pixels);
            ImGui::End();
        };
    }
    std::thread render_thread([&](){ if (renderer != nullptr) renderer->run(running); });
    u32 *image = (u32*)simd_aligned_malloc(SIMD_BYTES, sizeof(u32) * width * height);
    struct timespec start, end;

    vec4f_t origin{ 0.f, 0.f, -4.f };
    camera_t cam(origin.to_glm(), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 1.f), -90.f, 0.f, width, height);
    f32 *xs = cam.projection_plane.xs, *ys = cam.projection_plane.ys;
    f32 angle = -100.f;

    while (running)
    {
        cam.position = glm::vec3(glm::rotate_slow(glm::mat4(1.f), glm::radians(10.f), glm::vec3(0.f, 1.f, 0.f)) * glm::vec4(cam.position, 1.f));
        cam.turn(angle, 0.f);
        angle -= 10.f;
        origin = vec4f_t::from_glm(glm::vec4(cam.position, 0.f));

        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        gaussians.gaussians = staging_gaussians;
        gaussians.gaussians_broadcast->load_gaussians(staging_gaussians);
        tiles_t tiles = tile_gaussians(2.f/cmd.tiles, 2.f/cmd.tiles, staging_gaussians, cam.view_matrix);
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        tiling_time = simd::timeSpecDiffNsec(end, start)/1000000.f;
        //if (use_spline_approx) _erf = approx::spline_erf;
        //else if (use_mirror_approx) _erf = approx::spline_erf_mirror;
        //else if (use_taylor_approx) _erf = approx::taylor_erf;
        //else if (use_abramowitz_approx) _erf = approx::abramowitz_stegun_erf;
        //else _erf = std::erf;
        //if (use_fast_exp) _exp = approx::fast_exp;
        //else _exp = std::exp;
        //if (use_simd_transmittance) _transmittance = simd_transmittance;
        //else { _transmittance = transmittance; }

        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        bool res = false;
        if (use_tiling)
        {
            if (use_simd_pixels) res = simd_render_image(width, height, image, cam, origin, tiles, running, cmd.thread_count);
            else if (use_simd_transmittance) res =render_image(width, height, image, xs, ys, origin, tiles, running, cmd.thread_count); 
            else res = render_image(width, height, image, xs, ys, origin, tiles, running, cmd.thread_count, transmittance<decltype(expf), decltype(erff)>, expf, erff);
        }
        else
        {
            if (use_simd_pixels) res = simd_render_image(width, height, image, xs, ys, origin, gaussians, running);
            else if (use_simd_transmittance) res = render_image(width, height, image, xs, ys, origin, gaussians, running);
            else res = render_image(width, height, image, xs, ys, origin, gaussians, running, transmittance<decltype(expf), decltype(erff)>, expf, erff);
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        draw_time = simd::timeSpecDiffNsec(end, start)/1000000.f;
        if (res) break;

        if (cmd.outfile != nullptr)
            stbi_write_png(cmd.outfile, width, height, 4, image, width * 4);
        if (cmd.quiet)
        {
            fmt::println("TIME: {} ms", draw_time);
            break;
        }
        renderer->stage_image(image, width, height);
    }

    render_thread.join();
    simd_aligned_free(image);

    return EXIT_SUCCESS;
}
