#include <cmath>
#include <cstdlib>
#include <fmt/core.h>
#include <thread>
#include <vk-renderer/vk-renderer.h>
#include <include/error_fmt.h>
#include <sys/wait.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <include/stb_image_write.h>

#pragma GCC diagnostic pop

#include "types.h"

/// generated using spline interpolation from -4 to 4 in steps of .7
static float spline_erf(float x)
{
    if (x <= -3.3f) return -1.f;
    else if (-3.3f <= x && x <= -2.6f) { return -0.000112409376f * (x - -3.3f) * (x - -3.3f) * (x - -3.3f) + -0.00023605968f * (x - -3.3f) * (x - -3.3f) + -0.00010581506f * (x - -3.3f) + -0.99999696f; }
    else if (-2.6f <= x && x <= -1.9f) { return 0.0012324096f * (x - -2.6f) * (x - -2.6f) * (x - -2.6f) + 0.0023520004f * (x - -2.6f) * (x - -2.6f) + 0.0013753435f * (x - -2.6f) + -0.99976397f; }
    else if (-1.9f <= x && x <= -1.2f) { return 0.014164185f * (x - -1.9f) * (x - -1.9f) * (x - -1.9f) + 0.032096792f * (x - -1.9f) * (x - -1.9f) + 0.025489498f * (x - -1.9f) + -0.9927904f; }
    else if (-1.2f <= x && x <= -0.5f) { return 0.14258419f * (x - -1.2f) * (x - -1.2f) * (x - -1.2f) + 0.33152357f * (x - -1.2f) * (x - -1.2f) + 0.28002375f * (x - -1.2f) + -0.91031396f; }
    else if (-0.5f <= x && x <= 0.2f) { return 0.09140208f * (x - -0.5f) * (x - -0.5f) * (x - -0.5f) + 0.52346796f * (x - -0.5f) * (x - -0.5f) + 0.87851787f * (x - -0.5f) + -0.5204999f; }
    else if (0.2f <= x && x <= 0.9f) { return -0.37393388f * (x - 0.2f) * (x - 0.2f) * (x - 0.2f) + -0.2617932f * (x - 0.2f) * (x - 0.2f) + 1.0616902f * (x - 0.2f) + 0.2227026f; }
    else if (0.9f <= x && x <= 1.6f) { return -0.11865551f * (x - 0.9f) * (x - 0.9f) * (x - 0.9f) + -0.51096976f * (x - 0.9f) * (x - 0.9f) + 0.5207561f * (x - 0.9f) + 0.7969082f; }
    else if (1.6f <= x && x <= 2.3f) { return 0.19033842f * (x - 1.6f) * (x - 1.6f) * (x - 1.6f) + -0.11125909f * (x - 1.6f) * (x - 1.6f) + 0.085195914f * (x - 1.6f) + 0.9763484f; }
    else if (2.3f <= x && x <= 3.0f) { return 0.05069461f * (x - 2.3f) * (x - 2.3f) * (x - 2.3f) + -0.004800401f * (x - 2.3f) * (x - 2.3f) + 0.0039542736f * (x - 2.3f) + 0.99885684f; }
    else if (3.0f <= x && x <= 3.7f) { return 0.0020562427f * (x - 3.0f) * (x - 3.0f) * (x - 3.0f) + -0.00048229168f * (x - 3.0f) * (x - 3.0f) + 0.0002563885f * (x - 3.0f) + 0.9999779f; }
    return 1.f;
}

/// use half of the spline approximation and mirror it w.r.t. the origin
static float spline_erf_mirror(float x)
{
    float sign = x < 0 ? 1.f : -1.f;
    float value = -1.f;
    x = x > 0 ? -x : x; // take the negative absolute value
    if (-3.3f <= x && x <= -2.6f) { value = -0.000112409376f * (x - -3.3f) * (x - -3.3f) * (x - -3.3f) + -0.00023605968f * (x - -3.3f) * (x - -3.3f) + -0.00010581506f * (x - -3.3f) + -0.99999696f; }
    else if (-2.6f <= x && x <= -1.9f) { value = 0.0012324096f * (x - -2.6f) * (x - -2.6f) * (x - -2.6f) + 0.0023520004f * (x - -2.6f) * (x - -2.6f) + 0.0013753435f * (x - -2.6f) + -0.99976397f; }
    else if (-1.9f <= x && x <= -1.2f) { value = 0.014164185f * (x - -1.9f) * (x - -1.9f) * (x - -1.9f) + 0.032096792f * (x - -1.9f) * (x - -1.9f) + 0.025489498f * (x - -1.9f) + -0.9927904f; }
    else if (-1.2f <= x && x <= -0.5f) { value = 0.14258419f * (x - -1.2f) * (x - -1.2f) * (x - -1.2f) + 0.33152357f * (x - -1.2f) * (x - -1.2f) + 0.28002375f * (x - -1.2f) + -0.91031396f; }
    else if (-0.5f <= x && x <= 0.f) { value = 0.09140208f * (x - -0.5f) * (x - -0.5f) * (x - -0.5f) + 0.52346796f * (x - -0.5f) * (x - -0.5f) + 0.87851787f * (x - -0.5f) + -0.5204999f; }
    return value * sign;
}

static float taylor_erf(float x)
{
    if (x <= -2.f) return -1.f;
    if (x >= 2.f) return 1.f;
    return 2/std::sqrt(M_PIf) * (x + 1/3.f * -std::pow(x, 3.f) + 1/10.f * std::pow(x, 5.f) + 1/42.f * -std::pow(x, 7.f));
}

static float (*_erf)(float) = std::erf;

static float transmittance(const vec4f_t o, const vec4f_t n, const float s, const std::vector<gaussian_t> gaussians)
{
    float T = 0.f;
    for (const gaussian_t &g_q : gaussians)
    {
        const float mu_bar = (g_q.mu - o).dot(n);
        const float c_bar = g_q.magnitude * std::exp(-((g_q.mu - o).dot(g_q.mu - o) - std::pow(mu_bar, 2.f))/(2*std::pow(g_q.sigma, 2.f)));
        T += ((g_q.sigma * c_bar)/(std::sqrt(2/M_PIf))) * (_erf(-mu_bar/(std::sqrt(2) * g_q.sigma)) - _erf((s - mu_bar)/(std::sqrt(2) * g_q.sigma)));
    }
    return std::exp(T);
}

static float transmittance_step(const vec4f_t o, const vec4f_t n, const float s, const float delta, const std::vector<gaussian_t> gaussians)
{
    float T = 0.f;
    for (float t = 0; t <= s; t += delta)
    {
        for (const gaussian_t &g : gaussians)
            T += delta * g.pdf(o + n * t);
    }
    return std::exp(-T);
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

static bool approx_transmittance = false;
static bool new_approx_transmittance = approx_transmittance;

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
            if (approx_transmittance)
                T = transmittance_step(o, n, s, lambda_q, gaussians);
            else
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
    for (char *s : args)
        if (std::strncmp(s, "--grid", 7) == 0) grid = true;

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
    
    pid_t cpid[2];
    if ((cpid[0] = fork()) == 0)
    {
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
        char *args[] = { (char*)"julia", (char*)"--project=./julia", (char*)"./julia/transmittance.jl", NULL };
        execvp("julia", args);
        fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((cpid[1] = fork()) == 0)
    {
        FILE *CSV = std::fopen("csv/timing.csv", "wd");
        if (!CSV) exit(EXIT_FAILURE);
        fmt::println(CSV, "count, time");
        std::vector<gaussian_t> growing;
        for (u8 i = 0; i < 32; ++i)
        {
            for (u8 j = 0; j < 32; ++j)
            {
                growing.push_back(gaussian_t{
                        .albedo{ 1.f, 0.f, 0.f, 1.f },
                        .mu{ -1.f + i * 1.f/16.f, -1.f + j * 1.f/16.f, 0.f },
                        .sigma = 1.f/16.f,
                        .magnitude = 1.f
                        });
                auto start_time = std::chrono::high_resolution_clock::now();
                l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
                auto end_time = std::chrono::high_resolution_clock::now();
                float t = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
                fmt::println(CSV, "{}, {}", i * 32 + j + 1, t);
            }
        }
        std::fclose(CSV);
        char *args[] = { (char*)"julia", (char*)"--project=./julia", (char*)"./julia/timing.jl", NULL };
        execvp("julia", args);
        fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    // TODO: Add timing measurement for different erf implementation (spline vs. taylor vs. std::erf)
    if (fork() == 0)
    {
        FILE *CSV = std::fopen("csv/erf.csv", "wd");
        if (!CSV) exit(EXIT_FAILURE);
        fmt::println(CSV, "x, spline, spline_mirror, taylor, erf");
        for (float x = -6.f; x <= 6.f; x+=0.5f)
        {
            fmt::println(CSV, "{}, {}, {}, {}, {}", x, spline_erf(x), spline_erf_mirror(x), taylor_erf(x), std::erf(x));
        }
        // TODO: Add script to plot curves and err w.r.t. std::err
        exit(EXIT_SUCCESS);
    }

    renderer_t renderer;
    float draw_time = 0.f;
    bool use_spline_approx = false;
    bool use_mirror_approx = false;
    bool use_taylor_approx = false;
    if (!renderer.init(width, height, "Test")) return EXIT_FAILURE;
    renderer.custom_imgui = [&](){
        ImGui::Begin("Gaussians");
        for (gaussian_t &g : staging_gaussians) g.imgui_controls();
        ImGui::End();
        ImGui::Begin("Debug");
        ImGui::Text("Draw Time: %f ms", draw_time);
        ImGui::Checkbox("spline", &use_spline_approx);
        ImGui::Checkbox("mirror", &use_mirror_approx);
        ImGui::Checkbox("taylor", &use_taylor_approx);
        ImGui::Checkbox("approx", &new_approx_transmittance);
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
        approx_transmittance = new_approx_transmittance;
        if (use_spline_approx) _erf = spline_erf;
        else if (use_mirror_approx) _erf = spline_erf_mirror;
        else if (use_taylor_approx) _erf = taylor_erf;
        else _erf = std::erf;

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
