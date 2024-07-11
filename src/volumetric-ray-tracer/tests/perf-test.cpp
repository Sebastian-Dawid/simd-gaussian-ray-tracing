#include <cstdlib>
#include <fmt/core.h>
#include <include/error_fmt.h>
#include <sys/wait.h>
#include <sched.h>
#include <include/definitions.h>

#include <malloc.h>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wuninitialized"

#include <include/tsimd_sh.H>
#include <include/TimeMeasurement.H>

#pragma GCC diagnostic pop

#include "../types.h"
#include "../rt.h"

int main(i32 argc, char **argv)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);

    CPU_SET(2, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    constexpr u64 width = 512, height = 512;

    std::vector<gaussian_t> _gaussians;
    constexpr u8 grid_dim = 16;
    for (u8 i = 0; i < grid_dim; ++i)
        for (u8 j = 0; j < grid_dim; ++j)
            _gaussians.push_back(gaussian_t {
                    .albedo{ 1.f - (i * grid_dim + j)/(f32)(grid_dim*grid_dim), 0.f, 0.f + (i * grid_dim + j)/(f32)(grid_dim*grid_dim), 1.f },
                    .mu{ -1.f + 1.f/grid_dim + i * 1.f/(grid_dim/2.f), -1.f + 1.f/grid_dim + j * 1.f/(grid_dim/2.f), 1.f },
                    .sigma = 1.f/4.f,
                    .magnitude = 3.f
                    });


    gaussians_t gaussians{ .gaussians = _gaussians, .gaussians_broadcast = gaussian_vec_t::from_gaussians(_gaussians) };
    
    u32 *image = (u32*)simd_aligned_malloc(SIMD_BYTES, sizeof(u32) * width * height);
    //u32 *bg_image = (u32*)simd_aligned_malloc(SIMD_BYTES, sizeof(u32) * width * height);

    //bool w = false, h = false;
    //for (size_t x = 0; x < width; ++x)
    //{
    //    if (!(x % 16)) w = !w;
    //    for (size_t y = 0; y < height; ++y)
    //    {
    //        if (!(y % 16)) h = !h;
    //        bg_image[y * width + x] = (w ^ h) ? 0xFFAAAAAA : 0xFF555555;
    //    }
    //}

    f32 *xs, *ys;
    GENERATE_PROJECTION_PLANE(xs, ys, width, height);
    const vec4f_t origin{0.f, 0.f, 0.f};
    tiles_t tiles = tile_gaussians(.5f, .5f, _gaussians, glm::mat4(1));

    u32 v = -1;
    if (argc >= 2) v = atoi(argv[1]);
    switch (v)
    {
        case 0:
                render_image(width, height, image, xs, ys, origin, gaussians);
            break;
        case 1:
                render_image(width, height, image, xs, ys, origin, tiles, true, 32);
            break;
        case 2:
                simd_render_image(width, height, image, xs, ys, origin, gaussians);
            break;
        default:
                simd_render_image(width, height, image, xs, ys, origin, tiles, true, 32);
            break;
    }

    simd_aligned_free(image);
    //simd_aligned_free(bg_image);
    simd_aligned_free(xs);
    simd_aligned_free(ys);

    return EXIT_SUCCESS;
}
