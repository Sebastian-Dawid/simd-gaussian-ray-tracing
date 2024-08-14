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

#include <vrt/vrt.h>
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
    u64 nr_frames = 1;
    u64 tiles = 4;
    bool use_tiling = true;
    bool use_simd_transmittance = false;
    bool use_simd_l_hat = false;
    bool use_simd_pixels = true;
    f32 rot = 360.f;
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
            { "mode", required_argument, NULL, 'm' },
            { "frames", required_argument, NULL, 's' },
            {"rotation", required_argument, NULL, 'r' }
        };
        i32 lidx;
        while (1)
        {
            char c = getopt_long(argc, argv, "r:s:m:qw:o:f:g:h:t:", opts, &lidx);
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
                case 's':
                    this->nr_frames = strtoul(optarg, NULL, 10);
                    break;
                case 'r':
                    this->rot = strtof(optarg, NULL);
                    break;
                case 'm':
                    u64 mode = strtoul(optarg, NULL, 10);
                    this->use_tiling = false;
                    this->use_simd_transmittance = false;
                    this->use_simd_pixels = false;
                    switch (mode) {
                        case 5: // tiling sequential
                            this->use_tiling = true;
                        case 1: // no tiling sequential
                            break;
                        case 6: // tiling transmittance
                            this->use_tiling = true;
                        case 2: // no tiling transmittance
                            this->use_simd_transmittance = true;
                            break;
                        case 7: // tiling pixels
                            this->use_tiling = true;
                        case 3: // no tiling pixels
                            this->use_simd_l_hat = true;
                            break;
                        default:
                        case 8:
                            this->use_tiling = true;
                        case 4:
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
    bool use_simd_l_hat = cmd.use_simd_l_hat;
    bool use_simd_pixels = cmd.use_simd_pixels;
    bool use_tiling = cmd.use_tiling;

    u64 width = cmd.w, height = cmd.h;
    bool running = true;

    f32 angle_change = 0.f;
    f32 last_frame = 1000.f * glfwGetTime();
    f32 current_frame = 0.f;

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
            ImGui::Text("Frame Time: %f", current_frame);
            ImGui::Text("FPS: %f", 1000.f / current_frame);
            ImGui::Checkbox("use tiling", &use_tiling);
            ImGui::Checkbox("use parallel transmittance", &use_simd_transmittance);
            ImGui::Checkbox("use parallel radiance", &use_simd_l_hat);
            ImGui::Checkbox("use parallel pixels", &use_simd_pixels);
            ImGui::End();
        };
    }
    std::thread render_thread([&](){ if (renderer != nullptr) renderer->run(running); });
    u32 *image = (u32*)simd_aligned_malloc(SIMD_BYTES, sizeof(u32) * width * height);
    struct timespec start, end;

    vec4f_t origin{ 0.f, 0.f, -4.f };
    camera_t cam(origin.to_glm(), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 1.f), -90.f, 0.f, width, height);
    f32 angle = -90.f;
    u64 frames = 0;

    while (running)
    {
        frames++;
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        gaussians.gaussians = staging_gaussians;
        gaussians.gaussians_broadcast->load_gaussians(staging_gaussians);
        tiles_t tiles = tile_gaussians(2.f/cmd.tiles, 2.f/cmd.tiles, staging_gaussians, cam.view_matrix);
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        tiling_time = simd::timeSpecDiffNsec(end, start)/1000000.f;

        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        bool res = false;
        if (use_tiling)
        {
            if (use_simd_pixels) res = simd_render_image(width, height, image, cam, origin, tiles, running, cmd.thread_count);
            else if (use_simd_l_hat)
            {
                constexpr auto &tr = broadcast_transmittance<decltype(simd::exp), decltype(approx::simd_abramowitz_stegun_erf)>;
                res = render_image(width, height, image, cam, origin, tiles, running, cmd.thread_count, simd_l_hat<decltype(tr), decltype(simd::exp), decltype(approx::simd_abramowitz_stegun_erf)>,
                        tr, simd::exp, approx::simd_abramowitz_stegun_erf);
            }
            else if (use_simd_transmittance) res = render_image(width, height, image, cam, origin, tiles, running, cmd.thread_count); 
            else
            {
                constexpr auto &tr = transmittance<decltype(expf), decltype(erff)>;
                res = render_image(width, height, image, cam, origin, tiles, running, cmd.thread_count, l_hat<decltype(tr), decltype(expf), decltype(erff)>,
                        tr, expf, erff);
            }
        }
        else
        {
            if (use_simd_pixels) res = simd_render_image(width, height, image, cam, origin, gaussians, running);
            else if (use_simd_l_hat)
            {
                constexpr auto &tr = broadcast_transmittance<decltype(simd::exp), decltype(approx::simd_abramowitz_stegun_erf)>;
                res = render_image(width, height, image, cam, origin, gaussians, running, simd_l_hat<decltype(tr), decltype(simd::exp), decltype(approx::simd_abramowitz_stegun_erf)>,
                        tr, simd::exp, approx::simd_abramowitz_stegun_erf);
            }
            else if (use_simd_transmittance) res = render_image(width, height, image, cam, origin, gaussians, running);
            else
            {
                constexpr auto &tr = transmittance<decltype(expf), decltype(erff)>;
                res = render_image(width, height, image, cam, origin, gaussians, running, l_hat<decltype(tr), decltype(expf), decltype(erff)>, tr, expf, erff);
            }
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        draw_time = simd::timeSpecDiffNsec(end, start)/1000000.f;
        if (res) break;

        if (cmd.outfile != nullptr)
        {
            std::string outfile = std::string(cmd.outfile).substr(0, std::string(cmd.outfile).find_last_of("."));
            fmt::println("{}", outfile);
            if (cmd.nr_frames > 1)
                outfile = fmt::format("{}_{}.{}", outfile, frames, cmd.outfile + outfile.length() + 1);
            else
                outfile = fmt::format("{}.{}", outfile, cmd.outfile + outfile.length() + 1);
            fmt::println("{}", outfile);
            stbi_write_png(outfile.c_str(), width, height, 4, image, width * 4);
        }
        if (cmd.quiet)
        {
            fmt::println("TIME: {} ms", draw_time + tiling_time);
            if (cmd.nr_frames == frames) break;
        }
        else
        {
            renderer->stage_image(image, width, height);
        }

        if (!cmd.quiet)
        {
            current_frame = 1000.f * glfwGetTime() - last_frame;
            last_frame += current_frame;
        }

        angle_change = (!cmd.quiet) ? ((current_frame)/1000.f) * 90.f : cmd.rot / cmd.nr_frames;
        cam.position = glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(angle_change), glm::vec3(0.f, 1.f, 0.f)) * glm::vec4(cam.position, 1.f));
        origin = vec4f_t::from_glm(glm::vec4(cam.position, 0.f));
        angle -= angle_change;
        cam.turn(angle, 0.f);
    }

    render_thread.join();
    simd_aligned_free(image);

    return EXIT_SUCCESS;
}
