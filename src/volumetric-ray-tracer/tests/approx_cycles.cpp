#include <include/definitions.h>
#include <include/error_fmt.h>

#include <vrt/approx.h>

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
        asm volatile("":::"memory");        \
        FUNC                                \
        asm volatile("":::"memory");        \
        END = rdpmc_read(&(CTX));           \
        ACC += END - START;                 \
    }                                       \
}

using namespace approx;

i32 main()
{
    cpu_set_t mask;
    CPU_ZERO(&mask);    
    CPU_SET(2, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
    FILE *CSV = std::fopen("csv/simd_erf.csv", "wd");
    fmt::println(CSV, "count, t_svml, t_simd_spline, t_simd_spline_mirror, t_simd_abramowitz, t_spline, t_spline_mirror, t_abramowitz, t_std");
    struct rdpmc_ctx ctx;
    u64 start, end, acc;

    srand48(SEED);

    if (rdpmc_open(PERF_COUNT_HW_CPU_CYCLES, &ctx) < 0) exit(EXIT_FAILURE);
    for (u64 i = 1; i <= COUNT; ++i)
    {
        f32 t[SIMD_FLOATS * i];
        f32 s[SIMD_FLOATS * i];
        for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
            t[j] = -6.f + 12.f * drand48();

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; j += SIMD_FLOATS)
                simd::storeu(s + j, svml_erf(simd::loadu(t + j))););
        f32 t_svml = (f32)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                simd::storeu(s + j, simd_spline_erf(simd::loadu(t + j))););
        f32 t_simd = (f32)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                simd::storeu(s + j, simd_spline_erf_mirror(simd::loadu(t + j))););
        f32 t_simd_mirror = (f32)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                simd::storeu(s + j, simd_abramowitz_stegun_erf(simd::loadu(t + j))););
        f32 t_simd_abramowitz = (f32)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s[j] = spline_erf(t[j]););
        f32 t_spline = (f32)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s[j] = spline_erf_mirror(t[j]););
        f32 t_spline_mirror = (f32)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s[j] = abramowitz_stegun_erf(t[j]););
        f32 t_abramowitz = (f32)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s[j] = std::erf(t[j]););
        f32 t_std = (f32)acc/ITER;

        fmt::println(CSV, "{}, {}, {}, {}, {}, {}, {}, {}, {}", SIMD_FLOATS * i, t_svml, t_simd, t_simd_mirror, t_simd_abramowitz, t_spline, t_spline_mirror, t_abramowitz, t_std);
    }
    std::fclose(CSV);

    CSV = std::fopen("csv/simd_exp.csv", "wd");
    FILE* dev_null = std::fopen("/dev/null", "wd");
    fmt::println(CSV, "count, t_vcl, t_svml, t_simd, t_spline, t_fast, t_simd_fast, t_std");
    for (u64 i = 1; i < 100; ++i)
    {
        f32 t[SIMD_FLOATS * i];
        f32 s_std[SIMD_FLOATS * i];
        f32 s_vcl[SIMD_FLOATS * i];
        f32 s_svml[SIMD_FLOATS * i];
        f32 s_simd[SIMD_FLOATS * i];
        f32 s_spline[SIMD_FLOATS * i];
        f32 s_fast[SIMD_FLOATS * i];
        f32 s_simd_fast[SIMD_FLOATS * i];
        for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
            t[j] = -10.f + 10.f * drand48();

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                simd::storeu(s_vcl + j, vcl_exp(simd::loadu(t + j))););
        f32 t_vcl = (f32)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                simd::storeu(s_svml + j, svml_exp(simd::loadu(t + j))););
        f32 t_svml = (f32)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                simd::storeu(s_simd + j, simd_spline_exp(simd::loadu(t + j))););
        f32 t_simd = (f32)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s_spline[j] = spline_exp(t[j]););
        f32 t_spline = (f32)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s_fast[j] = fast_exp(t[j]););
        f32 t_fast = (f32)acc/ITER;
        
        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                simd::storeu(s_simd_fast + j, simd_fast_exp(simd::loadu(t + j))););
        f32 t_simd_fast = (f32)acc/ITER;

        BENCHMARK(ctx, start, end, acc, for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                s_std[j] = std::exp(t[j]););
        f32 t_std = (f32)acc/ITER;

        fmt::println(dev_null, "{}, {}, {}, {}, {}, {}, {}", s_vcl[0], s_svml[0], s_simd[0], s_spline[0], s_fast[0], s_simd_fast[0], s_std[0]);
        fmt::println(CSV, "{}, {}, {}, {}, {}, {}, {}, {}", SIMD_FLOATS * i, t_vcl, t_svml, t_simd, t_spline, t_fast, t_simd_fast, t_std);
    }
    std::fclose(CSV);
    char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/simd_erf_timing.jl", NULL };
    execvp("./julia/wrapper.sh", args);
    fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
    exit(EXIT_SUCCESS);
}
