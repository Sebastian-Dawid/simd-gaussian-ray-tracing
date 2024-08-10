#include "../rt.h"
#include "../approx.h"
#include <include/definitions.h>
#include <include/error_fmt.h>
#include <time.h>
#include <include/TimeMeasurement.H>

#define GETTIME(ts) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &(ts))

int main()
{
    cpu_set_t mask;
    CPU_ZERO(&mask);

    // bind process to CPU4
    CPU_SET(2, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    FILE *CSV = std::fopen("csv/image_timing_dim.csv", "wd");
    if (!CSV) exit(EXIT_FAILURE);
    fmt::println(CSV, "pixels, seq, simd_inner, simd_pixels");
    std::vector<gaussian_t> _gaussians;
    constexpr u8 num = 4;
    for (u8 i = 0; i < num; ++i)
        for (u8 j = 0; j < num; ++j)
            _gaussians.push_back(gaussian_t{
                    .albedo{ 1.f - (i * num + j)/(f32)(num*num), 0.f, 0.f + (i * num + j)/(f32)(num*num), 1.f },
                    .mu{ -1.f + 1.f/num + i * 1.f/(num/2.f), -1.f + 1.f/num + j * 1.f/(num/2.f), 0.f },
                    .sigma = 1.f/(2 * num),
                    .magnitude = 3.f
                    });
    gaussians_t gaussians{ .gaussians = _gaussians, .gaussians_broadcast = gaussian_vec_t::from_gaussians(_gaussians) };
    struct timespec start, end;

    const vec4f_t origin{ 0.f, 0.f, 0.f };

    for (u64 dim = 16; dim <= 512; dim += 16)
    {
        u32 *image = (u32*)simd_aligned_malloc(SIMD_BYTES, sizeof(u32) * dim * dim);
        const camera_t cam(camera_create_info_t{
            .width = dim,
            .height = dim
        });

        GETTIME(start);
        render_image(dim, dim, image, cam, origin, gaussians, true, transmittance<f32(f32), f32(f32)>, approx::fast_exp, approx::abramowitz_stegun_erf);
        GETTIME(end);
        f32 t_seq = simd::timeSpecDiffNsec(end, start)/1000.f;

        GETTIME(start);
        render_image(dim, dim, image, cam, origin, gaussians, true,
                simd_transmittance<decltype(approx::simd_fast_exp), decltype(approx::simd_abramowitz_stegun_erf)>,
                approx::simd_fast_exp, approx::simd_abramowitz_stegun_erf);
        GETTIME(end);
        f32 t_simd_inner = simd::timeSpecDiffNsec(end, start)/1000.f;

        GETTIME(start);
        simd_render_image(dim, dim, image, cam, origin, gaussians, true,
                approx::simd_fast_exp, approx::simd_abramowitz_stegun_erf);
        GETTIME(end);
        f32 t_simd_pixels = simd::timeSpecDiffNsec(end, start)/1000.f;

        fmt::println(CSV, "{}, {}, {}, {}", dim, t_seq, t_simd_inner, t_simd_pixels);

        simd_aligned_free(image);
    }
    fclose(CSV);


    u64 dims[] = { 64, 128, 256, 512 };
    for (u64 dim : dims)
    {
        CSV = std::fopen(fmt::format("csv/image_timing_gauss_{}px.csv", dim).c_str(), "wd");
        if (!CSV) exit(EXIT_FAILURE);
        fmt::println(CSV, "count, t_simd_inner, t_simd_pixels");
        for (u8 num = 4; num <= 16; ++num)
        {
            _gaussians.clear();
            for (u8 i = 0; i < num; ++i)
                for (u8 j = 0; j < num; ++j)
                    _gaussians.push_back(gaussian_t{
                            .albedo{ 1.f - (i * num + j)/(f32)(num*num), 0.f, 0.f + (i * num + j)/(f32)(num*num), 1.f },
                            .mu{ -1.f + 1.f/num + i * 1.f/(num/2.f), -1.f + 1.f/num + j * 1.f/(num/2.f), 0.f },
                            .sigma = 1.f/(2 * num),
                            .magnitude = 3.f
                            });
            gaussians_t gaussians{ .gaussians = _gaussians, .gaussians_broadcast = gaussian_vec_t::from_gaussians(_gaussians) };

            u32 *image = (u32*)simd_aligned_malloc(SIMD_BYTES, sizeof(u32) * dim * dim);
            const camera_t cam(camera_create_info_t{
                .width = dim,
                .height = dim
            });

            GETTIME(start);
            render_image(dim, dim, image, cam, origin, gaussians, true,
                    simd_transmittance<decltype(approx::simd_fast_exp), decltype(approx::simd_abramowitz_stegun_erf)>,
                    approx::simd_fast_exp, approx::simd_abramowitz_stegun_erf);
            GETTIME(end);
            f32 t_simd_inner = simd::timeSpecDiffNsec(end, start)/1000.f;

            GETTIME(start);
            simd_render_image(dim, dim, image, cam, origin, gaussians, true,
                    approx::simd_fast_exp, approx::simd_abramowitz_stegun_erf);
            GETTIME(end);
            f32 t_simd_pixels = simd::timeSpecDiffNsec(end, start)/1000.f;

            fmt::println(CSV, "{}, {}, {}", num, t_simd_inner, t_simd_pixels);

            simd_aligned_free(image);
        }
        std::fclose(CSV);
    }


    char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/image_timing.jl", NULL };
    execvp("./julia/wrapper.sh", args);
    fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
    return EXIT_FAILURE;
}
