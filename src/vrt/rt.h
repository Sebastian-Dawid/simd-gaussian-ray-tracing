#pragma once

#include <functional>
#include <thread>
#include <vector>
#include "types.h"
#include "thread-pool.h"
#include "camera.h"
#include "approx.h"
#include <include/tsimd_sh.H>
#include <glm/glm.hpp>

#define PRINT_MAT(V) fmt::println("{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} {}", V[0].x, V[1].x, V[2].x, V[3].x, V[0].y, V[1].y, V[2].y, V[3].y, V[0].z, V[1].z, V[2].z, V[3].z, V[0].w, V[1].w, V[2].w, V[3].w);

namespace vrt
{
    constexpr f32 SQRT_2_PI = 0.7978845608028654f;
    constexpr f32 SQRT_2 = 1.4142135623730951f;

    typedef f32(*f32_func_t)(f32);
    typedef simd::Vec<simd::Float>(*simd_f32_func_t)(simd::Vec<simd::Float>);
    typedef vec4f_t(*radiance_func_t)(const vec4f_t, const vec4f_t, const gaussians_t&);
    typedef f32(*transmittance_func_t)(const vec4f_t, const vec4f_t, const f32, const gaussians_t&);

    /// Calculates the transmittance at point s*n + o for the given gaussians.
    /// \param o the origin of the ray.
    /// \param n the direction of the ray. This should be a unit vector.
    /// \param s point along the ray to sample.
    /// \param gaussians the set of gaussians to compute the transmittance for.
    template<f32_func_t Exp = expf, f32_func_t Erf = erff>
    f32 transmittance(const vec4f_t o, const vec4f_t n, const f32 s, const gaussians_t &gaussians)
    {
        f32 T = 0.f;
        for (const gaussian_t &g_q : gaussians.gaussians)
        {
            const vec4f_t center_to_origin = g_q.mu - o;
            const f32 mu_bar = (center_to_origin).dot(n);
            const f32 c_bar = g_q.magnitude * Exp( -((center_to_origin).sqnorm() - std::pow(mu_bar, 2.f))/(2*std::pow(g_q.sigma, 2.f)) );
            const f32 sqrt_2_sig = SQRT_2 * g_q.sigma;
            T += ((g_q.sigma * c_bar)/(SQRT_2_PI)) * (Erf(-mu_bar/sqrt_2_sig) - Erf((s - mu_bar)/sqrt_2_sig));
        }
        return Exp(T);
    }

    /// Accelerated version of `transmittance` that operates on `SIMD_FLOATS` gaussians in parallel.
    /// \param o the origin of the ray.
    /// \param n the direction of the ray. This should be a unit vector.
    /// \param s point along the ray to sample.
    /// \param gaussians the set of gaussians to compute the transmittance for.
    template<simd_f32_func_t Exp = simd::exp, simd_f32_func_t Erf = simd::erf, f32_func_t Expf = expf>
    f32 simd_transmittance(const vec4f_t _o, const vec4f_t _n, const f32 s, const gaussians_t &gaussians)
    {
        simd::Vec<simd::Float> T = simd::set1(0.f);
        for (u64 i = 0; i < gaussians.gaussians.size(); i += SIMD_FLOATS)
        {
            simd_gaussian_t g_q{
                .albedo{},
                    .mu{ .x = simd::load(gaussians.soa_gaussians->mu.x + i),
                        .y = simd::load(gaussians.soa_gaussians->mu.y + i),
                        .z = simd::load(gaussians.soa_gaussians->mu.z + i) },
                    .sigma = simd::load(gaussians.soa_gaussians->sigma + i),
                    .magnitude = simd::load(gaussians.soa_gaussians->magnitude + i)
            };
            simd_vec4f_t o = simd_vec4f_t::from_vec4f_t(_o);
            simd_vec4f_t n = simd_vec4f_t::from_vec4f_t(_n);
            const simd_vec4f_t center_to_origin = g_q.mu - o;
            const simd::Vec<simd::Float> mu_bar = (center_to_origin).dot(n);
            const simd::Vec<simd::Float> c_bar = g_q.magnitude * Exp(-((center_to_origin).sqnorm() - (mu_bar * mu_bar))/(simd::set1(2.f) * g_q.sigma * g_q.sigma));

            const simd::Vec<simd::Float> sqrt_2_sig = simd::set1(SQRT_2) * g_q.sigma;
            T += ((g_q.sigma * c_bar)/simd::set1(SQRT_2_PI)) * (Erf(-mu_bar/sqrt_2_sig) - Erf((simd::set1(s) - mu_bar)/sqrt_2_sig));
        }
        return Expf(simd::hadds(T));
    }

    /// Version of `transmittance` that handles `SIMD_FLOATS` rays in parallel.
    /// \param o origins of the rays.
    /// \param n directions of the rays. These should be unit vectors.
    /// \param s points along the rays.
    /// \param gaussinas the set of gaussians to compute the transmittance for.
    template<simd_f32_func_t Exp = simd::exp, simd_f32_func_t Erf = simd::erf>
    simd::Vec<simd::Float> broadcast_transmittance(const simd_vec4f_t &o, const simd_vec4f_t &n, const simd::Vec<simd::Float> &s, const gaussians_t &gaussians)
    {
        simd::Vec<simd::Float> T = simd::set1(0.f);
        for (u64 i = 0; i < gaussians.gaussians.size(); ++i)
        {
            const simd_gaussian_t G_q = simd_gaussian_t::from_gaussian_t(gaussians.gaussians[i]);
            const simd_vec4f_t origin_to_center = G_q.mu - o;
            const simd::Vec<simd::Float> mu_bar = (origin_to_center).dot(n);
            const simd::Vec<simd::Float> c_bar = G_q.magnitude * Exp( -(((origin_to_center).sqnorm() - (mu_bar * mu_bar)) * simd::rcp(simd::set1(2.f) * G_q.sigma * G_q.sigma)) );

            const simd::Vec<simd::Float> sqrt_2_sig = simd::set1(SQRT_2) * G_q.sigma;

            const simd::Vec<simd::Float> mu_bar_sqrt_2_sig = mu_bar * simd::rcp(sqrt_2_sig);;
            const simd::Vec<simd::Float> s_sqrt_2_sig = s * simd::rcp(sqrt_2_sig);
            const simd::Vec<simd::Float> erf1 = Erf(-mu_bar_sqrt_2_sig);
            const simd::Vec<simd::Float> erf2 = Erf(s_sqrt_2_sig - mu_bar_sqrt_2_sig);
            T += ((G_q.sigma * c_bar) * simd::rcp(simd::set1(SQRT_2_PI))) * (erf1 - erf2);
        }
        return Exp(T);
    }

    /// Numerical approximation of the transmittance.
    f32 transmittance_step(const vec4f_t o, const vec4f_t n, const f32 s, const f32 delta, const std::vector<gaussian_t> gaussians);

    /// Returns the combined density at point `pt` for the given set of gaussians.
    f32 density(const vec4f_t pt, const std::vector<gaussian_t> gaussians);

    inline f32 (*_transmittance)(const vec4f_t, const vec4f_t, const f32, const gaussians_t&) = transmittance;

    /// Separate the given gaussians into sets based on which tiles of the image they affect.
    /// \param tw width of the image tiles.
    /// \param th height of the image tiles.
    /// \param gaussians the set of gaussians to separate.
    /// \param view the view matrix of the scene.
    tiles_t tile_gaussians(const f32 tw, const f32 th, const std::vector<gaussian_t> &gaussians, const glm::mat4 &view);

    /// Approximates the radiance integral L along the given ray o + s*n.
    /// \param o the origin of the ray.
    /// \param n the direction of the ray. This should be a unit vector.
    /// \param gaussians the gaussians to take into account for the computation.
    template<transmittance_func_t Tr = simd_transmittance>
    vec4f_t radiance(const vec4f_t o, const vec4f_t n, const gaussians_t &gaussians)
    {
        vec4f_t L_hat{ .x = 0.f, .y = 0.f, .z = 0.f };
        for (u64 i = 0; i < gaussians.gaussians.size(); ++i)
        {
            const gaussian_t &G_q = gaussians.gaussians[i];
            const f32 lambda_q = G_q.sigma;
            f32 inner = 0.f;
            for (i8 k = -4; k <= 0; ++k)
            {
                const f32 s = (G_q.mu - o).dot(n) + k * lambda_q;
                const f32 T = Tr(o, n, s, gaussians);
                inner += G_q.pdf(o + (n * s)) * T * lambda_q;
            }
            L_hat = L_hat + (G_q.albedo * inner);
        }
        return L_hat;
    }

    template<simd_f32_func_t Exp = simd::exp, simd_f32_func_t Erf = simd::erf>
    vec4f_t simd_radiance(const vec4f_t _o, const vec4f_t _n, const gaussians_t &gaussians)
    {
        vec4f_t L_hat{ 0.f, 0.f ,0.f };
        for (u64 i = 0; i < gaussians.gaussians.size(); i += SIMD_FLOATS)
        {
            simd_gaussian_t g_q{
                .albedo{
                    .x = simd::load(gaussians.soa_gaussians->albedo.r),
                        .y = simd::load(gaussians.soa_gaussians->albedo.g),
                        .z = simd::load(gaussians.soa_gaussians->albedo.b),
                        .w = simd::set1(1.f)
                },
                    .mu{
                        .x = simd::load(gaussians.soa_gaussians->mu.x + i),
                        .y = simd::load(gaussians.soa_gaussians->mu.y + i),
                        .z = simd::load(gaussians.soa_gaussians->mu.z + i) },
                    .sigma = simd::load(gaussians.soa_gaussians->sigma + i),
                    .magnitude = simd::load(gaussians.soa_gaussians->magnitude + i)
            };
            simd_vec4f_t o = simd_vec4f_t::from_vec4f_t(_o);
            simd_vec4f_t n = simd_vec4f_t::from_vec4f_t(_n);
            const simd::Vec<simd::Float> lambda = g_q.sigma;
            simd::Vec<simd::Float> inner = simd::set1(0.f);
            for (i8 k = -4; k <= 0; ++k)
            {
                const simd::Vec<simd::Float> s = (g_q.mu - o).dot(n) + simd::set1<simd::Float>(k) * lambda;
                const simd::Vec<simd::Float> T = broadcast_transmittance<Exp, Erf>(o, n, s, gaussians);
                inner += g_q.pdf(o + (n * s)) * T * lambda;
            }
            L_hat = L_hat + (g_q.albedo * inner).hadds();
        }
        return L_hat;
    }

    /// Version of `l_hat` that operates a set of `SIMD_FLOATS` rays.
    /// \param o the origins of the rays.
    /// \param n the directions of the rays. These should be unit vectors.
    /// \param gaussians the gaussians to take into account for the computation.
    template<simd_f32_func_t Exp = simd::exp, simd_f32_func_t Erf = simd::erf>
    simd_vec4f_t broadcast_radiance(const simd_vec4f_t o, const simd_vec4f_t n, const gaussians_t &gaussians)
    {
        simd_vec4f_t L_hat{ .x = simd::set1(0.f), .y = simd::set1(0.f), .z = simd::set1(0.f), .w = simd::set1(0.f) };
        for (const gaussian_t &_G_q : gaussians.gaussians)
        {
            const simd_gaussian_t G_q = simd_gaussian_t::from_gaussian_t(_G_q);
            const simd::Vec<simd::Float> lambda_q = G_q.sigma;
            simd::Vec<simd::Float> inner = simd::set1(0.f);
            for (i8 k = -4; k <= 0; ++k)
            {
                const simd::Vec<simd::Float> s = (G_q.mu - o).dot(n) + simd::set1((f32)k) * lambda_q;
                simd::Vec<simd::Float> T;
                T = broadcast_transmittance<Exp, Erf>(o, n, s, gaussians);
                inner += G_q.pdf(o + (n * s)) * T * lambda_q;
            }
            L_hat = L_hat + (G_q.albedo * inner);
        }
        return L_hat;
    }


    /// Renders an image with dimensions `width` x `height` of the given `gaussians` with the given `bg_image`  into `image`.
    /// The rays are defined in `xs` and `ys`.
    template<radiance_func_t Radiance = radiance>
    bool render_image(const u32 width, const u32 height, u32 *image, const camera_t &cam, const vec4f_t &origin, const gaussians_t &gaussians, const bool &running = true)
    {
        //static bool is_wayland_display = getenv("WAYLAND_DISPLAY") != NULL;

        for (u64 i = 0; i < width * height; ++i)
        {
            vec4f_t dir = vec4f_t{
                .x = cam.projection_plane.xs[i],
                .y = cam.projection_plane.ys[i],
                .z = cam.projection_plane.zs[i],
            } - origin;
            dir.normalize();
            const vec4f_t color = Radiance(origin, dir, gaussians);
            const u32 A = 0xFF000000; // final alpha channel is always 1
            const u32 R = (u32)(std::min(color.x, 1.0f) * 255);
            const u32 G = (u32)(std::min(color.y, 1.0f) * 255);
            const u32 B = (u32)(std::min(color.z, 1.0f) * 255);
            image[i] = A | R << 16 | G << 8 | B;
            //if (is_wayland_display)
            //    image[i] = A | R | G << 8 | B << 16;
            //else
            //    image[i] = A | R << 16 | G << 8 | B;
            if (!running) return true;
        }
        return false;
    }

    /// Renders an image with dimensions `width` x `height` of the given `gaussians` with the given `bg_image`  into `image`.
    /// The rays are defined in `xs` and `ys`.
    /// This version of the function takes a tiled set of gaussians.
    template<radiance_func_t Radiance = radiance>
    bool render_image(const u32 width, const u32 height, u32 *image, const camera_t &cam, const vec4f_t &origin, const tiles_t &tiles, const bool &running, const u64 tc)
    {
        const u64 tile_width = width * tiles.tw/2.f;
        const u64 tile_height = height * tiles.th/2.f;
        //static bool is_wayland_display = getenv("WAYLAND_DISPLAY") != NULL;

        std::vector<i32*> tile_buffers;
        {
            std::unique_ptr<thread_pool_t> tp = (tc == 1) ? nullptr : std::make_unique<thread_pool_t>(tc);
            for (u64 tidx = 0; tidx < tiles.w * tiles.h; ++tidx)
            {
                tile_buffers.push_back((i32*)simd_aligned_malloc(SIMD_BYTES, sizeof(i32) * tile_width * tile_height));
                /// NOTE: apparently i can not share the tiles object across multiple threads to access the gaussians
                //gaussians_t g{ tiles.gaussians[tidx].gaussians, new gaussian_vec_t(*tiles.gaussians[tidx].gaussians_broadcast) };
                std::function<void()> task = [img{tile_buffers[tidx]}, tidx, tile_width, tile_height,
                    g{gaussians_t{ tiles.gaussians[tidx].gaussians, new gaussian_vec_t(*(tiles.gaussians[tidx].soa_gaussians)) }},
                    &tiles, &cam, &origin] () {
                        for (u64 _i = 0; _i < tile_width * tile_height; ++_i)
                        {
                            const u64 i = (tidx % tiles.w) * tile_width + _i % tile_width // horizontal position
                                + (tile_width * tiles.w) * (_i/tile_width + (tidx/tiles.w) * tile_height); // vertical position
                            vec4f_t dir = vec4f_t{
                                .x = cam.projection_plane.xs[i],
                                .y = cam.projection_plane.ys[i],
                                .z = cam.projection_plane.zs[i]
                            } - origin;
                            dir.normalize();
                            const vec4f_t color = Radiance(origin, dir, g);
                            //simd::Vec<simd::Int> bg = simd::load(bg_image + i);
                            const u32 A = 0xFF000000; // final alpha channel is always 1
                            const u32 R = (u32)(std::min(color.x, 1.0f) * 255);
                            const u32 G = (u32)(std::min(color.y, 1.0f) * 255);
                            const u32 B = (u32)(std::min(color.z, 1.0f) * 255);
                            img[_i] = (A | R << 16 | G << 8 | B);
                            // if (is_wayland_display)
                            //     simd::store(img + _i, (A | simd::slli<16>(R) | simd::slli<8>(G) | B));
                            // else
                            //     simd::store(img + _i, (A | R | simd::slli<8>(G) | simd::slli<16>(B)));
                        }
                        delete g.soa_gaussians;
                    };
                if (tc == 1) {
                    task();
                    if (!running) return true;
                }
                else tp->enqueue(task);
            }
        } // NOTE: end of the scope implicitly joins threads through destructor

        for (u64 tidx = 0; tidx < tiles.w * tiles.h; ++tidx)
        {
            i32 *img = tile_buffers[tidx];
            for (u64 _i = 0; _i < tile_width * tile_height; _i += SIMD_FLOATS)
            {
                const u64 i = (tidx % tiles.w) * tile_width + _i % tile_width // horizontal position
                    + (tile_width * tiles.w) * (_i/tile_width + (tidx/tiles.w) * tile_height); // vertical positiona
                simd::Vec<simd::Int> d = simd::load(img + _i);
                simd::store((i32*)image + i, d);
            }
            simd_aligned_free(img);
        }
        tile_buffers.clear();
        if (!running) return true;
        return false;
    }

    /// Renders an image with dimensions `width` x `height` of the given `gaussians` with the given `bg_image`  into `image`.
    /// The rays are defined in `xs` and `ys`.
    /// This function is parallelized along the image pixels.
    /// Requires `image`, `xs` and `ys` to be aligned to `SIMD_BYTES`.
    template<simd_f32_func_t Exp = simd::exp, simd_f32_func_t Erf = simd::erf>
    bool simd_render_image(const u32 width, const u32 height, u32 *image, const camera_t &cam, const vec4f_t &origin, const gaussians_t &gaussians, const bool &running = true)
    {
        //static bool is_wayland_display = getenv("WAYLAND_DISPLAY") != NULL;
        const simd_vec4f_t simd_origin = simd_vec4f_t::from_vec4f_t(origin);

        for (u64 i = 0; i < width * height; i += SIMD_FLOATS)
        {
            simd_vec4f_t dir = simd_vec4f_t{
                .x = simd::load(cam.projection_plane.xs + i),
                    .y = simd::load(cam.projection_plane.ys + i),
                    .z = simd::load(cam.projection_plane.zs + i)
            } - simd_origin;
            dir.normalize();
            const simd_vec4f_t color = broadcast_radiance<Exp, Erf>(simd_origin, dir, gaussians);
            const simd::Vec<simd::Int> A = simd::set1<simd::Int>(0xFF000000);
            const simd::Vec<simd::Int> R = simd::cvts<simd::Int>(simd::min(color.x, simd::set1(1.f)) * simd::set1(255.f));
            const simd::Vec<simd::Int> G = simd::cvts<simd::Int>(simd::min(color.y, simd::set1(1.f)) * simd::set1(255.f));
            const simd::Vec<simd::Int> B = simd::cvts<simd::Int>(simd::min(color.z, simd::set1(1.f)) * simd::set1(255.f));
            simd::store((i32*)image + i, (A | simd::slli<16>(R) | simd::slli<8>(G) | B));
            //if (is_wayland_display)
            //    simd::store((i32*)image + i, (A | R | simd::slli<8>(G) | simd::slli<16>(B)));
            //else
            //    simd::store((i32*)image + i, (A | simd::slli<16>(R) | simd::slli<8>(G) | B));
            if (!running) return true;
        }
        return false;
    }

    /// Renders an image with dimensions `width` x `height` of the given `gaussians` with the given `bg_image`  into `image`.
    /// The rays are defined in `xs` and `ys`.
    /// This function is parallelized along the image pixels.
    /// Requires `image` to be aligned to `SIMD_BYTES`.
    /// This version of the function takes a tiled set of gaussians.
    /// The width of the tiles needs to be a multiple of `SIMD_FLOATS`.
    template<simd_f32_func_t Exp = simd::exp, simd_f32_func_t Erf = simd::erf>
    bool simd_render_image(const u32 width, const u32 height, u32 *image, const camera_t &cam, const vec4f_t origin, const tiles_t &tiles,
            const bool &running, const u64 tc)
    {
        const u64 tile_width = width * tiles.tw/2.f;
        const u64 tile_height = height * tiles.th/2.f;
        ASSERT((tile_width % SIMD_FLOATS == 0));
        //static bool is_wayland_display = getenv("WAYLAND_DISPLAY") != NULL;
        const simd_vec4f_t simd_origin = simd_vec4f_t::from_vec4f_t(origin);

        std::vector<i32*> tile_buffers;
        {
            std::unique_ptr<thread_pool_t> tp = (tc == 1) ? nullptr : std::make_unique<thread_pool_t>(tc);
            for (u64 tidx = 0; tidx < tiles.w * tiles.h; ++tidx)
            {
                tile_buffers.push_back((i32*)simd_aligned_malloc(SIMD_BYTES, sizeof(i32) * tile_width * tile_height));
                /// NOTE: apparently i can not share the tiles object across multiple threads to access the gaussians
                gaussians_t g{ tiles.gaussians[tidx].gaussians };
                std::function<void()> task = [img{tile_buffers[tidx]}, tidx, tile_width, tile_height, g, &tiles, &cam, &simd_origin] () {
                    for (u64 _i = 0; _i < tile_width * tile_height; _i += SIMD_FLOATS)
                    {
                        const u64 i = (tidx % tiles.w) * tile_width + _i % tile_width // horizontal position
                            + (tile_width * tiles.w) * (_i/tile_width + (tidx/tiles.w) * tile_height); // vertical position
                        simd_vec4f_t dir = simd_vec4f_t{
                            .x = simd::load(cam.projection_plane.xs + i),
                                .y = simd::load(cam.projection_plane.ys + i),
                                .z = simd::load(cam.projection_plane.zs + i)
                        } - simd_origin;
                        dir.normalize();
                        simd_vec4f_t color = broadcast_radiance<Exp, Erf>(simd_origin, dir, g);
                        simd::Vec<simd::Int> A = simd::set1<simd::Int>(0xFF000000);
                        simd::Vec<simd::Int> R = simd::cvts<simd::Int>(simd::min(simd::set1(1.f), color.x) * simd::set1(255.f));
                        simd::Vec<simd::Int> G = simd::cvts<simd::Int>(simd::min(simd::set1(1.f), color.y) * simd::set1(255.f));
                        simd::Vec<simd::Int> B = simd::cvts<simd::Int>(simd::min(simd::set1(1.f), color.z) * simd::set1(255.f));
                        simd::store(img + _i, (A | simd::slli<16>(R) | simd::slli<8>(G) | B));
                        // if (is_wayland_display)
                        //     simd::store(img + _i, (A | simd::slli<16>(R) | simd::slli<8>(G) | B));
                        // else
                        //     simd::store(img + _i, (A | R | simd::slli<8>(G) | simd::slli<16>(B)));
                    }
                };
                if (tc == 1) {
                    task();
                    if (!running) return true;
                }
                else tp->enqueue(task);
            }
        } // NOTE: end of the scope implicitly joins threads through destructor

        for (u64 tidx = 0; tidx < tiles.w * tiles.h; ++tidx)
        {
            i32 *img = tile_buffers[tidx];
            for (u64 _i = 0; _i < tile_width * tile_height; _i += SIMD_FLOATS)
            {
                const u64 i = (tidx % tiles.w) * tile_width + _i % tile_width // horizontal position
                    + (tile_width * tiles.w) * (_i/tile_width + (tidx/tiles.w) * tile_height); // vertical positiona
                simd::Vec<simd::Int> d = simd::load(img + _i);
                simd::store((i32*)image + i, d);
            }
            simd_aligned_free(img);
        }
        tile_buffers.clear();

        if (!running) return true;
        return false;
    }
};
