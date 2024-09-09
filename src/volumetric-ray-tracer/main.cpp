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

#define HELP_MSG "Usage: volumetric-ray-tracer [options]\n"\
    "\nOptions:\n"\
    "\t--help:                                 Show this help message.\n"\
    "\t--file <file>, -f <file>:               Load gaussians as verticies from <file> (.obj).\n"\
    "\t--output <file>, -o <file>:             Write image to <file> as in PNG format.\n"\
    "\t--grid <dim=4>, -g <dim=4>:             Render a grid of <dim>x<dim> gaussians. Overrides --file.\n"\
    "\t--width <width>, -w <width>:            Set image width to <width>. Set height to <width> too if --height is not set.\n"\
    "\t--height <height>, -h <height>:         Set image height to <height>. Set width to <height> too if --width is not set\n"\
    "\t--with-threads <count>, -t <count>:     Use <count> threads for rendering.\n"\
    "\t--quiet, -q:                            Quit after rendering without displaying the image to the screen.\n"\
    "\t--frames <count>:                       Render <count> frames. Does nothing if --quiet is not set.\n"\
    "\t--tiles <count>:                        Split the image into <count> tiles vertically and horizontally.\n"\
    "\t--rotation <rot>, -r <rot>:              Changes the viewing angle by <rot> every frame if --quiet is set.\n"\
    "\t--initial-rotation <rot>, -i <rot>:     Sets the initial rotation to <rot>.\n"\
    "\t--camaera-offset <offset>, -c <offset>: Set the position of the camera along the Z-Axis to <offset>.\n"\
    "\t--mode <mode>, -m <mode>:               Set the rendering mode to <mode>:\n"\
        "\t\t1 - sequential execution without tiling\n"\
        "\t\t2 - parallel transmittance calculation without tiling\n"\
        "\t\t3 - parallel radiance calculation without tiling\n"\
        "\t\t4 - parallel pixel calculation without tiling\n"\
        "\t\t5 - sequential execution with tiling\n"\
        "\t\t6 - parallel transmittance calculation with tiling\n"\
        "\t\t7 - parallel radiance calculation with tiling\n"\
        "\t\t8 - parallel pixel calculation with tiling"

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
    u64 tiles = 16;
    bool use_tiling = true;
    bool use_simd_transmittance = false;
    bool use_simd_l_hat = false;
    bool use_simd_pixels = true;
    f32 rot = 360.f;
    f32 inital_rot = 0.f;
    f32 camera_offset = -4.f;
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
            { "rotation", required_argument, NULL, 'r' },
            { "initial-rotation", required_argument, NULL, 'i'},
            { "camera-offset", required_argument, NULL, 'c' },
            { "help", no_argument, NULL, 0xff }
        };
        i32 lidx;
        while (1)
        {
            i32 c = getopt_long(argc, argv, "r:m:qw:o:f:g:h:t:c:i:", opts, &lidx);
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
                case 'i':
                    this->inital_rot = strtof(optarg, NULL);
                    break;
                case 'c':
                    this->camera_offset = strtof(optarg, NULL);
                    break;
                case 0xff:
                    fmt::println(HELP_MSG);
                    exit(EXIT_SUCCESS);
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
    std::vector<vrt::gaussian_t> _gaussians;
    if (cmd.use_grid)
    {
        u8 grid_dim = cmd.grid_dim;
        for (u8 i = 0; i < grid_dim; ++i)
            for (u8 j = 0; j < grid_dim; ++j)
                _gaussians.push_back(vrt::gaussian_t{
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
        _gaussians = { vrt::gaussian_t{ .albedo{ 0.f, 1.f, 0.f, 1.f }, .mu{ .3f, .3f, .5f }, .sigma = 0.1f, .magnitude = 2.f }, vrt::gaussian_t{ .albedo{ 0.f, 0.f, 1.f, 1.f }, .mu{ -.3f, -.3f, 1.f }, .sigma = 0.4f, .magnitude = .7f }, vrt::gaussian_t{ .albedo{ 1.f, 0.f, 0.f, 1.f }, .mu{ 0.f, 0.f, 2.f }, .sigma = .75f, .magnitude = 10.f } };
    }


    std::vector<vrt::gaussian_t> staging_gaussians = _gaussians;
    vrt::gaussians_t gaussians{ .gaussians = _gaussians, .soa_gaussians = vrt::gaussian_vec_t::from_gaussians(_gaussians) };
    
    std::unique_ptr<renderer_t> renderer = (cmd.quiet) ? nullptr : std::make_unique<renderer_t>();
    f32 draw_time = 0.f, tiling_time = 0.f, total_time = 0.f;
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
            for (vrt::gaussian_t &g : staging_gaussians) g.imgui_controls();
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

    vrt::vec4f_t origin{ 0.f, 0.f, cmd.camera_offset };
    vrt::camera_t cam(origin.to_glm(), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 1.f), -90.f, 0.f, width, height);
    f32 angle = -90.f;
    u64 frames = 0;
    cam.position = glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(cmd.inital_rot), glm::vec3(0.f, 1.f, 0.f)) * glm::vec4(cam.position, 1.f));
    origin = vrt::vec4f_t::from_glm(glm::vec4(cam.position, 0.f));
    angle -= cmd.inital_rot;
    cam.turn(angle, 0.f);

    while (running)
    {
        frames++;
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        gaussians.gaussians = staging_gaussians;
        gaussians.soa_gaussians->load_gaussians(staging_gaussians);
        vrt::tiles_t tiles = tile_gaussians(2.f/cmd.tiles, 2.f/cmd.tiles, staging_gaussians, cam.view_matrix);
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        tiling_time = simd::timeSpecDiffNsec(end, start)/1000000.f;

        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        bool res = false;
        if (use_tiling)
        {
            if (use_simd_pixels) res = vrt::simd_render_image(width, height, image, cam, origin, tiles, running, cmd.thread_count);
            else if (use_simd_l_hat)
            {
                res = vrt::render_image<vrt::simd_radiance>(width, height, image, cam, origin, tiles, running, cmd.thread_count);
            }
            else if (use_simd_transmittance) res = vrt::render_image(width, height, image, cam, origin, tiles, running, cmd.thread_count); 
            else
            {
                res = vrt::render_image<vrt::radiance<vrt::transmittance>>(width, height, image, cam, origin, tiles, running, cmd.thread_count);
            }
        }
        else
        {
            if (use_simd_pixels) res = vrt::simd_render_image(width, height, image, cam, origin, gaussians, running);
            else if (use_simd_l_hat)
            {
                res = vrt::render_image<vrt::simd_radiance>(width, height, image, cam, origin, gaussians, running);
            }
            else if (use_simd_transmittance) res = vrt::render_image(width, height, image, cam, origin, gaussians, running);
            else
            {
                res = vrt::render_image<vrt::radiance<vrt::transmittance>>(width, height, image, cam, origin, gaussians, running);
            }
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        draw_time = simd::timeSpecDiffNsec(end, start)/1000000.f;
        if (res) break;

        if (cmd.outfile != nullptr)
        {
            std::string outfile = std::string(cmd.outfile).substr(0, std::string(cmd.outfile).find_last_of("."));
            if (cmd.nr_frames > 1)
                outfile = fmt::format("{}_{}.{}", outfile, frames, cmd.outfile + outfile.length() + 1);
            else
                outfile = fmt::format("{}.{}", outfile, cmd.outfile + outfile.length() + 1);
            stbi_write_png(outfile.c_str(), width, height, 4, image, width * 4);
        }
        if (cmd.quiet)
        {
            if (cmd.nr_frames == 1) fmt::println("TIME: {} ms", draw_time + tiling_time);
            total_time += draw_time + tiling_time;
            if (cmd.nr_frames == frames)
            {
                if (cmd.nr_frames > 1)
                    fmt::println("AVG. TIME: {} ms ({} frames)", total_time/cmd.nr_frames, cmd.nr_frames);
                break;
            }
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
        origin = vrt::vec4f_t::from_glm(glm::vec4(cam.position, 0.f));
        angle -= angle_change;
        cam.turn(angle, 0.f);
    }

    render_thread.join();
    simd_aligned_free(image);

    return EXIT_SUCCESS;
}
