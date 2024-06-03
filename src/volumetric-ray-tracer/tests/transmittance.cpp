#include "../rt.h"
#include <include/error_fmt.h>
#include <sched.h>

int main()
{
    const std::vector<gaussian_t> _gaussians = { gaussian_t{ .albedo{ 0.f, 1.f, 0.f, .1f }, .mu{ .3f, .3f, .5f }, .sigma = 0.1f, .magnitude = 2.f }, gaussian_t{ .albedo{ 0.f, 0.f, 1.f, .7f }, .mu{ -.3f, -.3f, 0.f }, .sigma = 0.4f, .magnitude = .7f }, gaussian_t{ .albedo{ 1.f, 0.f, 0.f, 1.f }, .mu{ 0.f, 0.f, 2.f }, .sigma = .75f, .magnitude = 1.f } };
    gaussians_t gaussians{ .gaussians = _gaussians };
    const vec4f_t origin = { 0.f, 0.f, -5.f };
    
    cpu_set_t mask;
    CPU_ZERO(&mask);

    // bind process to CPU2
    CPU_SET(6, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    vec4f_t dir = {0.f, 0.f, 1.f};        
    FILE *CSV = std::fopen("csv/data.csv", "wd");
    if (!CSV) exit(EXIT_FAILURE);
    fmt::println(CSV, "s, T, T_s, err, D");
    for (float k = -6.f; k <= 6; k += .1f)
    {
        float s = (gaussians.gaussians[2].mu - origin).dot(dir) + k * gaussians.gaussians[2].sigma;
        float T = transmittance(origin, dir, s, gaussians);
        float T_s = transmittance_step(origin, dir, s, gaussians.gaussians[2].sigma, gaussians.gaussians);
        float err = std::abs(T - T_s);
        float D = density(origin + dir * s, gaussians.gaussians);
        fmt::println(CSV, "{}, {}, {}, {}, {}", s, T, T_s, err, D);
    }
    std::fclose(CSV);
    char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/transmittance.jl", NULL };
    execvp("./julia/wrapper.sh", args);
    fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
    exit(EXIT_FAILURE);
}
