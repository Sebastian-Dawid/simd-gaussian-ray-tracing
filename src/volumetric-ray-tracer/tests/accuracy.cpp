#include <include/error_fmt.h>
#include "../approx.h"
#include "include/definitions.h"
#include <sched.h>

int main()
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(10, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
    
    FILE *CSV = std::fopen("csv/erf.csv", "wd");
    if (!CSV) exit(EXIT_FAILURE);
    fmt::println(CSV, "x, spline, spline_mirror, taylor, abramowitz-stegun, erf");
    for (f32 x = -6.f; x <= 6.f; x+=0.1f)
    {
        fmt::println(CSV, "{}, {}, {}, {}, {}, {}", x, spline_erf(x), spline_erf_mirror(x), taylor_erf(x), abramowitz_stegun_erf(x), std::erf(x));
    }
    fclose(CSV);

    CSV = std::fopen("csv/exp.csv", "wd");
    if (!CSV) exit(EXIT_FAILURE);
    fmt::println(CSV, "x, exp");
    f32 t[160] = {0};
    f32 v_std[160] = {0};
    f32 v_fast[160] = {0};
    f32 v_fast_simd[160] = {0};
    u64 idx = 0;
    for (f32 x = -16.f; x <= 0.f; x += SIMD_FLOATS * 0.1)
    {
        for (u8 i = 0; i < SIMD_FLOATS; ++i)
        {
            t[idx + i] = x + 0.1f * i;
            v_std[idx + i] = std::exp(t[idx + i]);
            v_fast[idx + i] = fast_exp(t[idx + i]);
        }
        simd::storeu(v_fast_simd + idx, simd_fast_exp(simd::loadu(t + idx)));
        idx += SIMD_FLOATS;
    }
    for (u8 i = 0; i < 160; ++i)
    {
        fmt::println(CSV, "{}, {}, {}, {}", t[i], v_std[i], v_fast[i], v_fast_simd[i]); 
    }
    fclose(CSV);

    char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/cmp_erf.jl", NULL };
    execvp("./julia/wrapper.sh", args);
    fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
    exit(EXIT_FAILURE);
}
