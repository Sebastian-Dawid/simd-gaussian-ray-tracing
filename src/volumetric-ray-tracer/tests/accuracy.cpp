#include <include/error_fmt.h>
#include "../approx.h"
#include <sched.h>

int main()
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(6, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
    FILE *CSV = std::fopen("csv/erf.csv", "wd");
    if (!CSV) exit(EXIT_FAILURE);
    fmt::println(CSV, "x, spline, spline_mirror, taylor, abramowitz-stegun, erf");
    for (float x = -6.f; x <= 6.f; x+=0.1f)
    {
        fmt::println(CSV, "{}, {}, {}, {}, {}, {}", x, spline_erf(x), spline_erf_mirror(x), taylor_erf(x), abramowitz_stegun_erf(x), std::erf(x));
    }
    fclose(CSV);
    char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/cmp_erf.jl", NULL };
    execvp("./julia/wrapper.sh", args);
    fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
    exit(EXIT_FAILURE);
}
