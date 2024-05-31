#include "../rt.h"
#include "../approx.h"
#include <include/definitions.h>
#include <include/error_fmt.h>
#include <time.h>
#include <include/TimeMeasurement.H>

#define GETTIME(ts) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &(ts))

int main()
{
    const vec4f_t origin = { 0.f, 0.f, -5.f };
    cpu_set_t mask;
    CPU_ZERO(&mask);

    // bind process to CPU4
    CPU_SET(2, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    FILE *CSV = std::fopen("csv/image_timing.csv", "wd");
    if (!CSV) exit(EXIT_FAILURE);
    fmt::println(CSV, "pixels, seq, simd_inner, simd_pixels");
    std::vector<gaussian_t> _gaussians;
    constexpr u8 num = 4;
    for (u8 i = 0; i < num; ++i)
        for (u8 j = 0; j < num; ++j)
            _gaussians.push_back(gaussian_t{
                    .albedo{ 1.f - (i * num + j)/(float)(num*num), 0.f, 0.f + (i * num + j)/(float)(num*num), 1.f },
                    .mu{ -1.f + 1.f/num + i * 1.f/(num/2.f), -1.f + 1.f/num + j * 1.f/(num/2.f), 0.f },
                    .sigma = 1.f/(2 * num),
                    .magnitude = 3.f
                    });
    gaussians_t gaussians{ .gaussians = _gaussians, .gaussians_broadcast = gaussian_vec_t::from_gaussians(_gaussians) };
    struct timespec start, end;
    
    _simd_erf = simd_abramowitz_stegun_erf;
    _simd_exp = simd_fast_exp;
    _erf = abramowitz_stegun_erf;
    _exp = fast_exp;

    for (u64 dim = 16; dim <= 512; dim += 16)
    {
        float *xs = (float*)simd_aligned_malloc(SIMD_BYTES, sizeof(float) * dim * dim);
        float *ys = (float*)simd_aligned_malloc(SIMD_BYTES, sizeof(float) * dim * dim);

        for (u64 i = 0; i < dim; ++i)
        {
            for (u64 j = 0; j < dim; ++j)
            {
                xs[i * dim + j] = -1.f + j/(dim/2.f);
                ys[i * dim + j] = -1.f + i/(dim/2.f);
            }
        }

        _transmittance = transmittance;
        GETTIME(start);
        for (u64 i = 0; i < dim * dim; ++i)
        {
            const vec4f_t pt{xs[i], ys[i], 0.f};
            vec4f_t dir = pt - origin;
            dir.normalize();
            vec4f_t color = l_hat(origin, dir, gaussians);
        }
        GETTIME(end);
        float t_seq = simd::timeSpecDiffNsec(end, start)/1000.f;

        _transmittance = simd_transmittance;
        GETTIME(start);
        for (u64 i = 0; i < dim * dim; ++i)
        {
            const vec4f_t pt{xs[i], ys[i], 0.f};
            vec4f_t dir = pt - origin;
            dir.normalize();
            vec4f_t color = l_hat(origin, dir, gaussians);
        }
        GETTIME(end);
        float t_simd_inner = simd::timeSpecDiffNsec(end, start)/1000.f;

        GETTIME(start);
        for (u64 i = 0; i < dim * dim; i += SIMD_FLOATS)
        {
            static simd_vec4f_t simd_origin = simd_vec4f_t::from_vec4f_t(origin);
            simd_vec4f_t dir = simd_vec4f_t{ .x = simd::load(xs + i), .y = simd::load(ys + i), .z = simd::set1(0.f) } - simd_origin;
            dir.normalize();
            const simd_vec4f_t color = simd_l_hat(simd_origin, dir, gaussians);
        }
        GETTIME(end);
        float t_simd_pixels = simd::timeSpecDiffNsec(end, start)/1000.f;

        fmt::println(CSV, "{}, {}, {}, {}", dim, t_seq, t_simd_inner, t_simd_pixels);

        simd_aligned_free(xs);
        simd_aligned_free(ys);
    }
    fclose(CSV);

    char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/image_timing.jl", NULL };
    execvp("./julia/wrapper.sh", args);
    fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
    return EXIT_FAILURE;
}
