#include "../rt.h"
#include "../approx.h"
#include <include/definitions.h>
#include <include/error_fmt.h>
#include <time.h>
#include <include/TimeMeasurement.H>

#define GETTIME(ts) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &(ts))

using namespace approx;

int main()
{
    const vec4f_t origin = { 0.f, 0.f, -5.f };
    cpu_set_t mask;
    CPU_ZERO(&mask);

    // bind process to CPU4
    CPU_SET(4, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    FILE *CSV = std::fopen("csv/timing_erf.csv", "wd");
    if (!CSV) exit(EXIT_FAILURE);
    fmt::println(CSV, "count, t_spline, t_mirror, t_taylor, t_abramowitz, t_std");
    gaussians_t growing;
    struct timespec start, end;
    for (u8 i = 0; i < 16; ++i)
    {
        for (u8 j = 0; j < 16; ++j)
        {
            growing.gaussians.push_back(gaussian_t{
                    .albedo{ 1.f, 0.f, 0.f, 1.f },
                    .mu{ -1.f + i * 1.f/8.f, -1.f + j * 1.f/8.f, 0.f },
                    .sigma = 1.f/8.f,
                    .magnitude = 1.f
                    });
            _erf = spline_erf;
            GETTIME(start);
            l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
            GETTIME(end);
            u64 t_spline = simd::timeSpecDiffNsec(end, start)/1000.f;

            _erf = spline_erf_mirror;
            GETTIME(start);
            l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
            GETTIME(end);
            u64 t_mirror = simd::timeSpecDiffNsec(end, start)/1000.f;

            _erf = taylor_erf;
            GETTIME(start);
            l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
            GETTIME(end);
            u64 t_taylor = simd::timeSpecDiffNsec(end, start)/1000.f;
            
            _erf = abramowitz_stegun_erf;
            GETTIME(start);
            l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
            GETTIME(end);
            u64 t_abramowitz = simd::timeSpecDiffNsec(end, start)/1000.f;

            _erf = std::erf;
            GETTIME(start);
            l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
            GETTIME(end);
            u64 t_std = simd::timeSpecDiffNsec(end, start)/1000.f;

            fmt::println(CSV, "{}, {}, {}, {}, {}, {}", i * 16 + j + 1, t_spline, t_mirror, t_taylor, t_abramowitz, t_std);
        }
    }
    std::fclose(CSV);

    CSV = std::fopen("csv/timing_exp.csv", "wd");
    if (!CSV) exit(EXIT_FAILURE);
    fmt::println(CSV, "count, t_spline, t_fast, t_std");
    growing.gaussians.clear();
    _erf = std::erf;
    for (u8 i = 0; i < 16; ++i)
    {
        for (u8 j = 0; j < 16; ++j)
        {
            growing.gaussians.push_back(gaussian_t{
                    .albedo{ 1.f, 0.f, 0.f, 1.f },
                    .mu{ -1.f + i * 1.f/8.f, -1.f + j * 1.f/8.f, 0.f },
                    .sigma = 1.f/8.f,
                    .magnitude = 1.f
                    });
            _exp = spline_exp;
            GETTIME(start);
            l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
            GETTIME(end);
            f32 t_spline = simd::timeSpecDiffNsec(end, start)/1000.f;
            
            _exp = fast_exp;
            GETTIME(start);
            l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
            GETTIME(end);
            f32 t_fast = simd::timeSpecDiffNsec(end, start)/1000.f;

            _exp = std::exp;
            GETTIME(start);
            l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
            GETTIME(end);
            f32 t_std = simd::timeSpecDiffNsec(end, start)/1000.f;

            fmt::println(CSV, "{}, {}, {}, {}", i * 16 + j + 1, t_spline, t_fast, t_std);
        }
    }
    std::fclose(CSV);

    CSV = std::fopen("csv/timing_transmittance.csv", "wd");
    if (!CSV) exit(EXIT_FAILURE);
    fmt::println(CSV, "count, t_std, t_simd");
    growing.gaussians_broadcast = gaussian_vec_t::from_gaussians(growing.gaussians);
    growing.gaussians.clear();
    _erf = std::erf;
    for (u8 i = 0; i < 16; ++i)
    {
        for (u8 j = 0; j < 16; ++j)
        {
            growing.gaussians.push_back(gaussian_t{
                    .albedo{ 1.f, 0.f, 0.f, 1.f },
                    .mu{ -1.f + i * 1.f/8.f, -1.f + j * 1.f/8.f, 0.f },
                    .sigma = 1.f/8.f,
                    .magnitude = 1.f
                    });
            growing.gaussians_broadcast->load_gaussians(growing.gaussians);
            _simd_erf = simd_abramowitz_stegun_erf;
            _simd_exp = simd_fast_exp;
            _transmittance = simd_transmittance;
            GETTIME(start);
            l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
            GETTIME(end);
            f32 t_spline = simd::timeSpecDiffNsec(end, start)/1000.f;
            
            _erf = abramowitz_stegun_erf;
            _exp = fast_exp;
            _transmittance = transmittance;
            GETTIME(start);
            l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
            GETTIME(end);
            f32 t_fast = simd::timeSpecDiffNsec(end, start)/1000.f;

            fmt::println(CSV, "{}, {}, {}", i * 16 + j + 1, t_spline, t_fast);
        }
    }
    std::fclose(CSV);

    char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/timing.jl", NULL };
    execvp("./julia/wrapper.sh", args);
    fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
    exit(EXIT_FAILURE);
}
