#pragma once

#include <vector>
#include "types.h"
#include <include/tsimd_sh.H>
#include <glm/glm.hpp>

#define GENERATE_PROJECTION_PLANE(XS, YS, W, H)                        \
{                                                                      \
    XS = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * W * H);   \
    YS = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * W * H);   \
    for (u64 i = 0; i < H; ++i)                                        \
    {                                                                  \
        for (u64 j = 0; j < W; ++j)                                    \
        {                                                              \
            xs[i * W + j] = -1.f + j/(W/2.f);                          \
            ys[i * W + j] = -1.f + i/(H/2.f);                          \
        }                                                              \
    }                                                                  \
}

f32 transmittance(const vec4f_t o, const vec4f_t n, const f32 s, const gaussians_t &gaussians);
f32 simd_transmittance(const vec4f_t o, const vec4f_t n, const f32 s, const gaussians_t &gaussians);
f32 transmittance_step(const vec4f_t o, const vec4f_t n, const f32 s, const f32 delta, const std::vector<gaussian_t> gaussians);
f32 density(const vec4f_t pt, const std::vector<gaussian_t> gaussians);

inline f32 (*_transmittance)(const vec4f_t, const vec4f_t, const f32, const gaussians_t&) = transmittance;

tiles_t tile_gaussians(const f32 tw, const f32 th, const std::vector<gaussian_t> &gaussians, const glm::mat4 &view);

vec4f_t l_hat(const vec4f_t o, const vec4f_t n, const gaussians_t &gaussians);
simd_vec4f_t simd_l_hat(const simd_vec4f_t o, const simd_vec4f_t n, const gaussians_t &gaussians);

bool render_image(const u32 width, const u32 height, u32 *image, const u32 *bg_image, const f32 *xs, const f32 *ys, const gaussians_t &gaussians, const bool &running = true);
bool render_image(const u32 width, const u32 height, u32 *image, const u32 *bg_image, const f32 *xs, const f32 *ys, const tiles_t &tiles, const bool &running = true);

/// requires `image`, `xs` and `ys` to be aligned to SIMD_BYTES
bool simd_render_image(const u32 width, const u32 height, u32 *image, const i32 *bg_image, const f32 *xs, const f32 *ys, const gaussians_t &gaussians, const bool &running = true);
bool simd_render_image(const u32 width, const u32 height, u32 *image, const i32 *bg_image, const f32 *xs, const f32 *ys, const tiles_t &gaussians, const bool &running = true);
