#include <vrt/vrt.h>
#include "include/error_fmt.h"
#define VCL_NAMESPACE vcl
#include <include/vectorclass/vectormath_exp.h>

#define SQ(X, Y, Z) (X)*(X) + (Y)*(Y) + (Z)*(Z)

using namespace vrt;

i32 main()
{
    cpu_set_t mask;
    CPU_ZERO(&mask);

    CPU_SET(2, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
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
    tiles_t tiles = tile_gaussians(1.f/8.f, 1.f/8.f, _gaussians, glm::mat4(1.f));

    u32 *ref_image = (u32*)simd::aligned_malloc(sizeof(u32) * 256 * 256);
    const camera_t cam(camera_create_info_t{});

    const vec4f_t origin{0.f, 0.f, 0.f};
    fmt::print("[ {} ]\tGenerating Reference Image\n", INFO_FMT("INFO"));
    render_image<radiance<transmittance>>(256, 256, ref_image, cam, origin, tiles, true, 16);

    fmt::print("[ {} ]\tGenerating Test Images\n", INFO_FMT("INFO"));
    u32 *svml_image = (u32*)simd::aligned_malloc(sizeof(u32) * 256 * 256);
    u32 *fog_image = (u32*)simd::aligned_malloc(sizeof(u32) * 256 * 256);
    u32 *my_image = (u32*)simd::aligned_malloc(sizeof(u32) * 256 * 256);

    simd_render_image(256, 256, svml_image, cam, origin, tiles, true, 16);
    simd_render_image<approx::vcl_exp, approx::simd_abramowitz_stegun_erf>(256, 256, fog_image, cam, origin, tiles, true, 16);
    simd_render_image<approx::simd_fast_exp, approx::simd_abramowitz_stegun_erf>(256, 256, my_image, cam, origin, tiles, true, 16);

    double svml_err = 0.0, fog_err = 0.0, my_err = 0.0;
    for (u64 i = 0; i < 256 * 256; ++i)
    {
        float R = (ref_image[i] & 0xFF)/255.f, G = ((ref_image[i] & 0xFF00) >> 8)/255.f, B = ((ref_image[i] & 0xFF0000) >> 16)/255.f;
        float svml_R = (svml_image[i] & 0xFF)/255.f, svml_G = ((svml_image[i] & 0xFF00) >> 8)/255.f, svml_B = ((svml_image[i] & 0xFF0000) >> 16)/255.f;
        float fog_R = (fog_image[i] & 0xFF)/255.f, fog_G = ((fog_image[i] & 0xFF00) >> 8)/255.f, fog_B = ((fog_image[i] & 0xFF0000) >> 16)/255.f;
        float my_R = (my_image[i] & 0xFF)/255.f, my_G = ((my_image[i] & 0xFF00) >> 8)/255.f, my_B = ((my_image[i] & 0xFF0000) >> 16)/255.f;
        svml_err += SQ((R - svml_R), (G - svml_G), (B - svml_B));
        fog_err += SQ((R - fog_R), (G - fog_G), (B - fog_B));
        my_err += SQ((R - my_R), (G - my_G), (B - my_B));
    }
    svml_err /= 256*256;
    fog_err /= 256*256;
    my_err /= 256*256;

    fmt::print("SVML: {}\nFOG:  {}\nMINE: {}\n", svml_err, fog_err, my_err);

    return EXIT_SUCCESS;
}
