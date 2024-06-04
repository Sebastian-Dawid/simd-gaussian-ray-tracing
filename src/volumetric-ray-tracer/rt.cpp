#include "rt.h"
#include <include/definitions.h>

constexpr f32 SQRT_2_PI = 0.7978845608028654f;
constexpr f32 SQRT_2 = 1.4142135623730951f;

f32 transmittance(const vec4f_t o, const vec4f_t n, const f32 s, const gaussians_t &gaussians)
{
    f32 T = 0.f;
    for (const gaussian_t &g_q : gaussians.gaussians)
    {
        const f32 mu_bar = (g_q.mu - o).dot(n);
        const f32 c_bar = g_q.magnitude * _exp( -((g_q.mu - o).dot(g_q.mu - o) - std::pow(mu_bar, 2.f))/(2*std::pow(g_q.sigma, 2.f)) );
        T += ((g_q.sigma * c_bar)/(SQRT_2_PI)) * (_erf(-mu_bar/(SQRT_2 * g_q.sigma)) - _erf((s - mu_bar)/(SQRT_2 * g_q.sigma)));
    }
    return _exp(T);
}

f32 simd_transmittance(const vec4f_t _o, const vec4f_t _n, const f32 s, const gaussians_t &gaussians)
{
    simd::Vec<simd::Float> T = simd::set1(0.f);
    for (u64 i = 0; i < gaussians.gaussians.size(); i += SIMD_FLOATS)
    {
        simd_gaussian_t g_q{
            .albedo{},
            .mu{ .x = simd::load(gaussians.gaussians_broadcast.mu.x + i),
                .y = simd::load(gaussians.gaussians_broadcast.mu.y + i),
                .z = simd::load(gaussians.gaussians_broadcast.mu.z + i) },
            .sigma = simd::load(gaussians.gaussians_broadcast.sigma + i),
            .magnitude = simd::load(gaussians.gaussians_broadcast.magnitude + i)
        };
        simd_vec4f_t o = simd_vec4f_t::from_vec4f_t(_o);
        simd_vec4f_t n = simd_vec4f_t::from_vec4f_t(_n);
        const simd::Vec<simd::Float> mu_bar = (g_q.mu - o).dot(n);
        const simd::Vec<simd::Float> c_bar = g_q.magnitude * _simd_exp(-((g_q.mu - o).dot(g_q.mu - o) - (mu_bar * mu_bar))/(simd::set1(2.f) * g_q.sigma * g_q.sigma));

        T += ((g_q.sigma * c_bar)/simd::set1(SQRT_2_PI)) * (_simd_erf(-mu_bar/(simd::set1(SQRT_2) * g_q.sigma)) - _simd_erf((simd::set1(s) - mu_bar)/(simd::set1(SQRT_2) * g_q.sigma)));
    }
    return _exp(simd::hadds(T));
}

simd::Vec<simd::Float> broadcast_transmittance(const simd_vec4f_t &o, const simd_vec4f_t &n, const simd::Vec<simd::Float> &s, const gaussians_t &gaussians)
{
    simd::Vec<simd::Float> T = simd::set1(0.f);
    for (const gaussian_t &_G_q : gaussians.gaussians)
    {
        const simd_gaussian_t G_q = simd_gaussian_t::from_gaussian_t(_G_q);
        const simd::Vec<simd::Float> mu_bar = (G_q.mu - o).dot(n);
        const simd::Vec<simd::Float> c_bar = G_q.magnitude * _simd_exp( -((G_q.mu - o).dot(G_q.mu - o) - (mu_bar * mu_bar))/(simd::set1(2.f) * G_q.sigma * G_q.sigma) );

        T += ((G_q.sigma * c_bar)/simd::set1(SQRT_2_PI)) * (_simd_erf(-mu_bar/(simd::set1(SQRT_2) * G_q.sigma)) - _simd_erf((s - mu_bar)/(simd::set1(SQRT_2) * G_q.sigma)));
    }
    return _simd_exp(T);
}

f32 transmittance_step(const vec4f_t o, const vec4f_t n, const f32 s, const f32 delta, const std::vector<gaussian_t> gaussians)
{
    f32 T = 0.f;
    for (f32 t = 0; t <= s; t += delta)
    {
        for (const gaussian_t &g : gaussians)
            T += delta * g.pdf(o + n * t);
    }
    return _exp(-T);
}

f32 density(const vec4f_t pt, const std::vector<gaussian_t> gaussians)
{
    f32 D = 0.f;
    for (const gaussian_t &g : gaussians)
    {
        D += g.pdf(pt);
    }
    return D;
}

vec4f_t l_hat(const vec4f_t o, const vec4f_t n, const gaussians_t &gaussians)
{
    vec4f_t L_hat{ .x = 0.f, .y = 0.f, .z = 0.f };
    for (const gaussian_t &G_q : gaussians.gaussians)
    {
        const f32 lambda_q = G_q.sigma;
        f32 inner = 0.f;
        for (i8 k = -4; k <= 0; ++k)
        {
            const f32 s = (G_q.mu - o).dot(n) + k * lambda_q;
            f32 T;
            T = _transmittance(o, n, s, gaussians);
            inner += G_q.pdf(o + (n * s)) * T * lambda_q;
        }
        L_hat = L_hat + (G_q.albedo * inner);
    }
    return L_hat;
}

simd_vec4f_t simd_l_hat(const simd_vec4f_t o, const simd_vec4f_t n, const gaussians_t &gaussians)
{
    simd_vec4f_t L_hat{ .x = simd::set1(0.f), .y = simd::set1(0.f), .z = simd::set1(0.f) };
    for (const gaussian_t &_G_q : gaussians.gaussians)
    {
        const simd_gaussian_t G_q = simd_gaussian_t::from_gaussian_t(_G_q);
        const simd::Vec<simd::Float> lambda_q = G_q.sigma;
        simd::Vec<simd::Float> inner = simd::set1(0.f);
        for (i8 k = -4; k <= 0; ++k)
        {
            const simd::Vec<simd::Float> s = (G_q.mu - o).dot(n) + simd::set1((f32)k) * lambda_q;
            simd::Vec<simd::Float> T;
            T = broadcast_transmittance(o, n, s, gaussians);
            inner += G_q.pdf(o + (n * s)) * T * lambda_q;
        }
        L_hat = L_hat + (G_q.albedo * inner);
    }
    return L_hat;
}

bool render_image(const u32 width, const u32 height, u32 *image, const f32 *xs, const f32 *ys, const gaussians_t &gaussians, const bool &running)
{
    static bool is_wayland_display = getenv("WAYLAND_DISPLAY") != NULL;
    static const vec4f_t origin = { 0.f, 0.f, -5.f };

    for (u64 i = 0; i < width * height; ++i)
    {
        vec4f_t dir = vec4f_t{ xs[i], ys[i], 0.f } - origin;
        dir.normalize();
        vec4f_t color = l_hat(origin, dir, gaussians);
        u32 A = 0xFF000000; // final alpha channel is always 1
        u32 R = (u32)(color.x * 255);
        u32 G = (u32)(color.y * 255);
        u32 B = (u32)(color.z * 255);
        if (is_wayland_display)
            image[i] = A | R | G << 8 | B << 16;
        else
            image[i] = A | R << 16 | G << 8 | B;
        if (!running) return true;
    }
    return false;
}

bool simd_render_image(const u32 width, const u32 height, u32 *image, const f32 *xs, const f32 *ys, const gaussians_t &gaussians, const bool &running)
{
    static bool is_wayland_display = getenv("WAYLAND_DISPLAY") != NULL;
    static const simd_vec4f_t simd_origin = simd_vec4f_t::from_vec4f_t(vec4f_t{ 0.f, 0.f, -5.f });

    for (u64 i = 0; i < width * height; i += SIMD_FLOATS)
    {
        simd_vec4f_t dir = simd_vec4f_t{ .x = simd::load(xs + i), .y = simd::load(ys + i), .z = simd::set1(0.f) } - simd_origin;
        dir.normalize();
        const simd_vec4f_t color = simd_l_hat(simd_origin, dir, gaussians);
        simd::Vec<simd::Int> A = simd::set1<simd::Int>(0xFF000000);
        simd::Vec<simd::Int> R = simd::cvts<simd::Int>(color.x * simd::set1(255.f));
        simd::Vec<simd::Int> G = simd::cvts<simd::Int>(color.y * simd::set1(255.f));
        simd::Vec<simd::Int> B = simd::cvts<simd::Int>(color.z * simd::set1(255.f));
        if (is_wayland_display)
            simd::store((i32*)image + i, (A | R | simd::slli<8>(G) | simd::slli<16>(B)));
        else
            simd::store((i32*)image + i, (A | simd::slli<16>(R) | simd::slli<8>(G) | B));
        if (!running) return true;
    }
    return false;
}
