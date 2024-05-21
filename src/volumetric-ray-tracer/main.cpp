#include <cmath>
#include <cstdlib>
#include <fmt/core.h>
#include <thread>
#include <vk-renderer/vk-renderer.h>
#include <include/error_fmt.h>
#include <sys/wait.h>
#include <sched.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wuninitialized"

#include <include/tsimd_sh.H>
#include <include/TimeMeasurement.H>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <include/stb_image_write.h>

#pragma GCC diagnostic pop

#define barrier() __asm__ __volatile__("" : : : "memory")

#include "types.h"
#include "approx.h"

static float (*_erf)(float) = std::erf;
static float (*_exp)(float) = std::exp;

static float transmittance(const vec4f_t o, const vec4f_t n, const float s, const std::vector<gaussian_t> gaussians)
{
    float T = 0.f;
    for (const gaussian_t &g_q : gaussians)
    {
        const float mu_bar = (g_q.mu - o).dot(n);
        const float c_bar = g_q.magnitude * _exp(-((g_q.mu - o).dot(g_q.mu - o) - std::pow(mu_bar, 2.f))/(2*std::pow(g_q.sigma, 2.f)));
        T += ((g_q.sigma * c_bar)/(std::sqrt(2/M_PIf))) * (_erf(-mu_bar/(std::sqrt(2) * g_q.sigma)) - _erf((s - mu_bar)/(std::sqrt(2) * g_q.sigma)));
    }
    return _exp(T);
}

static float transmittance_step(const vec4f_t o, const vec4f_t n, const float s, const float delta, const std::vector<gaussian_t> gaussians)
{
    float T = 0.f;
    for (float t = 0; t <= s; t += delta)
    {
        for (const gaussian_t &g : gaussians)
            T += delta * g.pdf(o + n * t);
    }
    return _exp(-T);
}

static float density(const vec4f_t pt, const std::vector<gaussian_t> gaussians)
{
    float D = 0.f;
    for (const gaussian_t &g : gaussians)
    {
        D += g.pdf(pt);
    }
    return D;
}

static vec4f_t l_hat(const vec4f_t o, const vec4f_t n, const std::vector<gaussian_t> gaussians)
{
    vec4f_t L_hat{ .x = 0.f, .y = 0.f, .z = 0.f };
    for (const gaussian_t &G_q : gaussians)
    {
        const float lambda_q = G_q.sigma;
        float inner = 0.f;
        for (i8 k = -4; k <= 0; ++k)
        {
            const float s = (G_q.mu - o).dot(n) + k * lambda_q;
            float T;
            T = transmittance(o, n, s, gaussians);
            inner += G_q.pdf(o + (n * s)) * T * lambda_q;
        }
        L_hat = L_hat + (G_q.albedo * inner);
    }
    return L_hat;
}

int main(i32 argc, char **argv)
{
    u64 width = 256, height = 256;
    if (argc >= 3)
    {
        width = strtoul(argv[1], NULL, 10);
        if (width == 0)
        {
            fmt::println(stderr, "[ {} ]\tInvaid width set! Setting widht to 256.", ERROR_FMT("ERROR"));
            width = 256;
        }
        height = strtoul(argv[2], NULL, 10);
        if (height == 0)
        {
            fmt::println(stderr, "[ {} ]\tInvaid height set! Setting height to 256.", ERROR_FMT("ERROR"), strerror(errno));
            height = 256;
        }
    }
    u64 new_width = width, new_height = height;
    
    std::vector<char*> args(argc);
    std::memcpy(args.data(), argv, sizeof(*argv) * argc);
    bool grid = false;
    bool with_tests = true;
    for (char *s : args)
    {
        if (std::strncmp(s, "--grid", 7) == 0) grid = true;
        else if (std::strncmp(s, "--no-tests", 11) == 0) with_tests = false;
    }

    std::vector<gaussian_t> gaussians;
    if (grid)
    {
        for (u8 i = 0; i < 4; ++i)
            for (u8 j = 0; j < 4; ++j)
                gaussians.push_back(gaussian_t{
                        .albedo{ 1.f, 0.f, 0.f, 1.f },
                        .mu{ -1.f + i * 1.f/2.f, -1.f + j * 1.f/2.f, 0.f },
                        .sigma = 1.f/8.f,
                        .magnitude = 1.f
                        });
    }
    else
    {
        gaussians = { gaussian_t{ .albedo{ 0.f, 1.f, 0.f, .1f }, .mu{ .3f, .3f, .5f }, .sigma = 0.1f, .magnitude = 2.f }, gaussian_t{ .albedo{ 0.f, 0.f, 1.f, .7f }, .mu{ -.3f, -.3f, 0.f }, .sigma = 0.4f, .magnitude = .7f }, gaussian_t{ .albedo{ 1.f, 0.f, 0.f, 1.f }, .mu{ 0.f, 0.f, 2.f }, .sigma = .75f, .magnitude = 1.f } };
    }
    std::vector<gaussian_t> staging_gaussians = gaussians;
    const vec4f_t origin = { 0.f, 0.f, -5.f };
    
    if (with_tests)
    {
        cpu_set_t mask;
        CPU_ZERO(&mask);

        if (fork() == 0)
        {
            // bind process to CPU2
            CPU_SET(2, &mask);
            sched_setaffinity(0, sizeof(mask), &mask);

            vec4f_t dir = {0.f, 0.f, 1.f};        
            FILE *CSV = std::fopen("csv/data.csv", "wd");
            if (!CSV) exit(EXIT_FAILURE);
            fmt::println(CSV, "s, T, T_s, err, D");
            for (float k = -6.f; k <= 6; k += .1f)
            {
                float s = (gaussians[2].mu - origin).dot(dir) + k * gaussians[2].sigma;
                float T = transmittance(origin, dir, s, gaussians);
                float T_s = transmittance_step(origin, dir, s, gaussians[2].sigma, gaussians);
                float err = std::abs(T - T_s);
                float D = density(origin + dir * s, gaussians);
                fmt::println(CSV, "{}, {}, {}, {}, {}", s, T, T_s, err, D);
            }
            std::fclose(CSV);
            char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/transmittance.jl", NULL };
            execvp("./julia/wrapper.sh", args);
            fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (fork() == 0)
        {
            // bind process to CPU4
            CPU_SET(4, &mask);
            sched_setaffinity(0, sizeof(mask), &mask);

            FILE *CSV = std::fopen("csv/timing_erf.csv", "wd");
            if (!CSV) exit(EXIT_FAILURE);
            fmt::println(CSV, "count, t_spline, t_mirror, t_taylor, t_std");
            std::vector<gaussian_t> growing;
            for (u8 i = 0; i < 16; ++i)
            {
                for (u8 j = 0; j < 16; ++j)
                {
                    growing.push_back(gaussian_t{
                            .albedo{ 1.f, 0.f, 0.f, 1.f },
                            .mu{ -1.f + i * 1.f/8.f, -1.f + j * 1.f/8.f, 0.f },
                            .sigma = 1.f/8.f,
                            .magnitude = 1.f
                            });
                    _erf = spline_erf;
                    auto start_time = std::chrono::system_clock::now();
                    l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
                    auto end_time = std::chrono::system_clock::now();
                    float t_spline = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
                    
                    _erf = spline_erf_mirror;
                    start_time = std::chrono::system_clock::now();
                    l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
                    end_time = std::chrono::system_clock::now();
                    float t_mirror = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

                    _erf = taylor_erf;
                    start_time = std::chrono::system_clock::now();
                    l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
                    end_time = std::chrono::system_clock::now();
                    float t_taylor = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

                    _erf = std::erf;
                    start_time = std::chrono::system_clock::now();
                    l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
                    end_time = std::chrono::system_clock::now();
                    float t_std = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

                    fmt::println(CSV, "{}, {}, {}, {}, {}", i * 16 + j + 1, t_spline, t_mirror, t_taylor, t_std);
                }
            }
            std::fclose(CSV);
            
            CSV = std::fopen("csv/timing_exp.csv", "wd");
            if (!CSV) exit(EXIT_FAILURE);
            fmt::println(CSV, "count, t_spline, t_std");
            growing.clear();
            _erf = std::erf;
            for (u8 i = 0; i < 16; ++i)
            {
                for (u8 j = 0; j < 16; ++j)
                {
                    growing.push_back(gaussian_t{
                            .albedo{ 1.f, 0.f, 0.f, 1.f },
                            .mu{ -1.f + i * 1.f/8.f, -1.f + j * 1.f/8.f, 0.f },
                            .sigma = 1.f/8.f,
                            .magnitude = 1.f
                            });
                    _exp = spline_exp;
                    auto start_time = std::chrono::system_clock::now();
                    l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
                    auto end_time = std::chrono::system_clock::now();
                    float t_spline = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

                    _exp = std::exp;
                    start_time = std::chrono::system_clock::now();
                    l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
                    end_time = std::chrono::system_clock::now();
                    float t_std = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

                    fmt::println(CSV, "{}, {}, {}", i * 16 + j + 1, t_spline, t_std);
                }
            }
            std::fclose(CSV);
            char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/timing.jl", NULL };
            execvp("./julia/wrapper.sh", args);
            fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (fork() == 0)
        {
            // bind process to CPU6
            CPU_SET(6, &mask);
            sched_setaffinity(0, sizeof(mask), &mask);
            FILE *CSV = std::fopen("csv/erf.csv", "wd");
            if (!CSV) exit(EXIT_FAILURE);
            fmt::println(CSV, "x, spline, spline_mirror, taylor, erf");
            for (float x = -6.f; x <= 6.f; x+=0.1f)
            {
                fmt::println(CSV, "{}, {}, {}, {}, {}", x, spline_erf(x), spline_erf_mirror(x), taylor_erf(x), std::erf(x));
            }
            fclose(CSV);
            char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/cmp_erf.jl", NULL };
            execvp("./julia/wrapper.sh", args);
            fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    
    renderer_t renderer;
    float draw_time = 0.f;
    bool use_spline_approx = false;
    bool use_mirror_approx = false;
    bool use_taylor_approx = false;
    bool use_abramowitz_approx = false;
    bool use_fast_exp = false;
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
        ImGui::End();
    };
    bool running = true;
    std::thread render_thread([&](){ renderer.run(running); });

    u32 *image = (u32*)std::calloc(width * height, sizeof(u32));

    while (running)
    {
        if (new_width != width || new_height != height)
        {
            width = new_width;
            height = new_height;
            image = (u32*)std::realloc(image, width * height * sizeof(u32));
        }
        gaussians = staging_gaussians;
        if (use_spline_approx) _erf = spline_erf;
        else if (use_mirror_approx) _erf = spline_erf_mirror;
        else if (use_taylor_approx) _erf = taylor_erf;
        else if (use_abramowitz_approx) _erf = abramowitz_stegun_erf;
        else _erf = std::erf;
        if (use_fast_exp) _exp = fast_exp;
        else _exp = std::exp;

        auto start_time = std::chrono::system_clock::now();
        vec4f_t pt = { -1.f, -1.f, 0.f };
        for (u64 y = 0; y < height; ++y)
        {
            for (u64 x = 0; x < width; ++x)
            {
                vec4f_t dir = pt - origin;
                dir.normalize();
                vec4f_t color = l_hat(origin, dir, gaussians);
                u32 A = 0xFF000000; // final alpha channel is always 1
                u32 R = (u32)(color.x * 255);
                u32 G = (u32)(color.y * 255);
                u32 B = (u32)(color.z * 255);
                image[y * width + x] = A | R << 16 | G << 8 | B;
                pt.x += 1/(width/2.f);
                if (!running) goto cleanup;
            }
            pt.x = -1.f;
            pt.y += 1/(height/2.f);
        }

        auto end_time = std::chrono::system_clock::now();
        draw_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        if (argc >= 4)
            stbi_write_png(argv[3], width, height, 4, image, width * 4);
        renderer.stage_image(image, width, height);
    }

cleanup:
    render_thread.join();
    free(image);

    return EXIT_SUCCESS;
}
