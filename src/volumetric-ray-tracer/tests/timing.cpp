#include <include/definitions.h>
#include <include/error_fmt.h>

#include "../approx.h"

#include <include/TimeMeasurement.H>
#include <sched.h>

#include <rdpmc.h>

#ifndef ITER
#define ITER 1000
#endif

#ifndef COUNT
#define COUNT 200
#endif

#ifndef SEED
#define SEED 42
#endif

#define BENCHMARK(CTX,START,END,ACC,FUNC)   \
{                                           \
    ACC = 0;                                \
    for (u64 _ = 0; _ < ITER; ++_)          \
    {                                       \
        START = rdpmc_read(&(CTX));         \
        FUNC                                \
        END = rdpmc_read(&(CTX));           \
        ACC += END - START;                 \
    }                                       \
}

i32 main()
{
    cpu_set_t mask;
    CPU_ZERO(&mask);    
    // bind process to CPU8
    CPU_SET(8, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
    FILE *CSV = std::fopen("csv/simd_erf.csv", "wd");
    fmt::println(CSV, "count, t_simd, t_simd_mirror, t_simd_abramowitz, t_spline, t_spline_mirror, t_abramowitz, t_std");
    struct rdpmc_ctx ctx;
    u64 start, end, acc;

    srand48(SEED);

    if (rdpmc_open(PERF_COUNT_HW_CPU_CYCLES, &ctx) < 0) exit(EXIT_FAILURE);
    for (u64 i = 1; i <= COUNT; ++i)
    {
        float t[SIMD_FLOATS * i];
        float s[SIMD_FLOATS * i];
        for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
            t[j] = -6.f + 12.f * drand48();

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                simd::storeu(s + j, simd_spline_erf(simd::loadu(t + j))););
        float t_simd = (float)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                simd::storeu(s + j, simd_spline_erf_mirror(simd::loadu(t + j))););
        float t_simd_mirror = (float)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                simd::storeu(s + j, simd_abramowitz_stegun_erf(simd::loadu(t + j))););
        float t_simd_abramowitz = (float)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s[j] = spline_erf(t[j]););
        float t_spline = (float)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s[j] = spline_erf_mirror(t[j]););
        float t_spline_mirror = (float)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s[j] = abramowitz_stegun_erf(t[j]););
        float t_abramowitz = (float)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s[j] = std::erf(t[j]););
        float t_std = (float)acc/ITER;

        fmt::println(CSV, "{}, {}, {}, {}, {}, {}, {}, {}", SIMD_FLOATS * i, t_simd, t_simd_mirror, t_simd_abramowitz, t_spline, t_spline_mirror, t_abramowitz, t_std);
    }
    std::fclose(CSV);

    CSV = std::fopen("csv/simd_exp.csv", "wd");
    fmt::println(CSV, "count, t_simd, t_spline, t_fast, t_simd_fast, t_std");
    for (u64 i = 1; i < 100; ++i)
    {
        float t[SIMD_FLOATS * i];
        float s[SIMD_FLOATS * i];
        for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
            t[j] = -10.f + 10.f * drand48();

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                simd::storeu(s + j, simd_spline_exp(simd::loadu(t + j))););
        float t_simd = (float)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s[j] = spline_exp(t[j]););
        float t_spline = (float)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s[j] = fast_exp(t[j]););
        float t_fast = (float)acc/ITER;
        
        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                simd::storeu(s + j, simd_fast_exp(simd::loadu(t + j))););
        float t_simd_fast = (float)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s[j] = std::exp(t[j]););
        float t_std = (float)acc/ITER;

        fmt::println(CSV, "{}, {}, {}, {}, {}, {}", SIMD_FLOATS * i, t_simd, t_spline, t_fast, t_simd_fast, t_std);
    }
    std::fclose(CSV);
    char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/simd_erf_timing.jl", NULL };
    execvp("./julia/wrapper.sh", args);
    fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
    exit(EXIT_SUCCESS);
}
