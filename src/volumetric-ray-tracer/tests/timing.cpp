#include <include/definitions.h>
#include <include/error_fmt.h>

#include <chrono>

#include "../approx.h"

#include <include/TimeMeasurement.H>
#include <sched.h>

i32 main()
{
    cpu_set_t mask;
    CPU_ZERO(&mask);    
    // bind process to CPU8
    CPU_SET(8, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
    FILE *CSV = std::fopen("csv/simd_erf.csv", "wd");
    fmt::println(CSV, "count, t_simd, t_simd_mirror, t_spline, t_spline_mirror, t_std");
    for (u64 i = 1; i < 100; ++i)
    {
        float t[SIMD_FLOATS * i];
        float s[SIMD_FLOATS * i];
        for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
            t[j] = -4.f + j * 8.f/(SIMD_FLOATS * i);

        struct timespec start, end;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
        for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
            simd::storeu(s + j, simd_spline_erf(simd::loadu(t + j)));
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
        i64 t_simd = simd::timeSpecDiffNsec(end, start);

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
        for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
            simd::storeu(s + j, simd_spline_erf_mirror(simd::loadu(t + j)));
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
        i64 t_simd_mirror = simd::timeSpecDiffNsec(end, start);

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
        for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
            s[j] = spline_erf(t[j]);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
        i64 t_spline = simd::timeSpecDiffNsec(end, start);

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
        for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
            s[j] = spline_erf(t[j]);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
        i64 t_spline_mirror = simd::timeSpecDiffNsec(end, start);

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
        for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
            s[j] = std::erf(t[j]);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
        i64 t_std = simd::timeSpecDiffNsec(end, start);

        fmt::println(CSV, "{}, {}, {}, {}, {}, {}", SIMD_FLOATS * i, t_simd, t_simd_mirror, t_spline, t_spline_mirror, t_std);
    }
    std::fclose(CSV);

    CSV = std::fopen("csv/simd_exp.csv", "wd");
    fmt::println(CSV, "count, t_simd, t_spline, t_std");
    for (u64 i = 1; i < 100; ++i)
    {
        float t[SIMD_FLOATS * i];
        float s[SIMD_FLOATS * i];
        for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
            t[j] = -10.f + j * 10.f/(SIMD_FLOATS * i);

        struct timespec start, end;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
        for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
            simd::storeu(s + j, simd_spline_exp(simd::loadu(t + j)));
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
        i64 t_simd = simd::timeSpecDiffNsec(end, start);

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
        for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
            s[j] = spline_exp(t[j]);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
        i64 t_spline = simd::timeSpecDiffNsec(end, start);

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
        for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
            s[j] = std::exp(t[j]);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
        i64 t_std = simd::timeSpecDiffNsec(end, start);

        fmt::println(CSV, "{}, {}, {}, {}", SIMD_FLOATS * i, t_simd, t_spline, t_std);
    }
    std::fclose(CSV);
    char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/simd_erf_timing.jl", NULL };
    execvp("./julia/wrapper.sh", args);
    fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
    exit(EXIT_FAILURE);
}
