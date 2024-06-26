#include "../approx.h"
#include <include/tsimd_sh.H>
#include <include/definitions.h>
#include <include/error_fmt.h>
#include <time.h>
#include <include/TimeMeasurement.H>

#define GETTIME(ts) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &(ts))

constexpr u64 ITER  = 100000;
constexpr u64 COUNT = 1024;
constexpr u64 SEED  = 42;

extern "C" __m512 __svml_expf16(__m512);
extern "C" __m512 __svml_erff16(__m512);

i32 main()
{
    cpu_set_t mask;
    CPU_ZERO(&mask);    
    CPU_SET(2, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    srand(SEED);
    float _x[COUNT], res_svml[COUNT], res_mine[COUNT], res_real[COUNT];
    for (u64 i = 0; i < COUNT; ++i)
    {
        _x[i] = drand48() * -180.0;
        res_real[i] = std::exp(_x[i]);
    }
    

    fmt::println("EXP:\n  TIMING:");
    struct timespec start, end;
    {
        GETTIME(start);
        for (u64 i = 0; i < ITER; ++i)
        {
            for (u64 j = 0; j < COUNT; j += SIMD_FLOATS)
            {
                __m512 x = _mm512_loadu_ps(_x + j);
                x = __svml_expf16(x);
                _mm512_storeu_ps(res_svml + j, x);
            }
        } 
        GETTIME(end);
        fmt::println("    SVML: {} ns", simd::timeSpecDiffNsec(end, start));
    }
    {
        GETTIME(start);
        for (u64 i = 0; i < ITER; ++i)
        {
            for (u64 j = 0; j < COUNT; j += SIMD_FLOATS)
            {
                simd::Vec<simd::Float> x = simd::loadu(_x + j);
                x = simd_fast_exp(x);
                simd::storeu(res_mine + j, x);
            }
        } 
        GETTIME(end);
        fmt::println("    MINE: {} ns", simd::timeSpecDiffNsec(end, start));
    }

    FILE *CSV = std::fopen("csv/simd_svml_exp.csv", "wd");
    fmt::println(CSV, "x, real, SVML, MINE");
    for (u64 i = 0; i < COUNT; ++i)
    {
        fmt::println(CSV, "{}, {}, {}, {}", _x[i], res_real[i], res_svml[i], res_mine[i]);
    }
    std::fclose(CSV);

    double svml_err = 0.0, my_err = 0.0;
    for (u64 i = 0; i < COUNT; i += SIMD_FLOATS)
    {
        simd::Vec<simd::Float> real = simd::loadu(res_real + i);
        svml_err += simd::hadds(simd::absDiff(simd::loadu(res_svml + i), real));
        my_err += simd::hadds(simd::absDiff(simd::loadu(res_mine + i), real));
    }
    svml_err /= COUNT;
    my_err /= COUNT;

    fmt::println("  ERRORS:\n    SVML: {}\n    MINE: {}", svml_err, my_err);

    for (u64 i = 0; i < COUNT; ++i)
    {
        _x[i] = drand48() * 7 - 3.5;
        res_real[i] = std::erf(_x[i]);
    }
    
    fmt::println("ERF:\n  TIMING:");
    {
        GETTIME(start);
        for (u64 i = 0; i < ITER; ++i)
        {
            for (u64 j = 0; j < COUNT; j += SIMD_FLOATS)
            {
                __m512 x = _mm512_loadu_ps(_x + j);
                x = __svml_erff16(x);
                _mm512_storeu_ps(res_svml + j, x);
            }
        } 
        GETTIME(end);
        fmt::println("    SVML: {} ns", simd::timeSpecDiffNsec(end, start));
    }
    {
        GETTIME(start);
        for (u64 i = 0; i < ITER; ++i)
        {
            for (u64 j = 0; j < COUNT; j += SIMD_FLOATS)
            {
                simd::Vec<simd::Float> x = simd::loadu(_x + j);
                x = simd_abramowitz_stegun_erf(x);
                simd::storeu(res_mine + j, x);
            }
        } 
        GETTIME(end);
        fmt::println("    MINE: {} ns", simd::timeSpecDiffNsec(end, start));
    }
    
    CSV = std::fopen("csv/simd_svml_erf.csv", "wd");
    fmt::println(CSV, "x, real, SVML, MINE");
    for (u64 i = 0; i < COUNT; ++i)
    {
        fmt::println(CSV, "{}, {}, {}, {}", _x[i], res_real[i], res_svml[i], res_mine[i]);
    }
    std::fclose(CSV);

    svml_err = 0.0;
    my_err = 0.0;
    for (u64 i = 0; i < COUNT; i += SIMD_FLOATS)
    {
        simd::Vec<simd::Float> real = simd::loadu(res_real + i);
        svml_err += simd::hadds(simd::absDiff(simd::loadu(res_svml + i), real));
        my_err += simd::hadds(simd::absDiff(simd::loadu(res_mine + i), real));
    }
    svml_err /= COUNT;
    my_err /= COUNT;

    fmt::println("  ERRORS:\n    SVML: {}\n    MINE: {}", svml_err, my_err);

    return EXIT_SUCCESS;
}
