#include "rt.h"
#include <include/definitions.h>

constexpr float SQRT_2_PI = 0.7978845608028654f;
constexpr float SQRT_2 = 1.4142135623730951f;

float transmittance(const vec4f_t o, const vec4f_t n, const float s, const gaussians_t &gaussians)
{
    float T = 0.f;
    for (const gaussian_t &g_q : gaussians.gaussians)
    {
        const float mu_bar = (g_q.mu - o).dot(n);
        const float c_bar = g_q.magnitude * _exp( -((g_q.mu - o).dot(g_q.mu - o) - std::pow(mu_bar, 2.f))/(2*std::pow(g_q.sigma, 2.f)) );
        T += ((g_q.sigma * c_bar)/(SQRT_2_PI)) * (_erf(-mu_bar/(SQRT_2 * g_q.sigma)) - _erf((s - mu_bar)/(SQRT_2 * g_q.sigma)));
    }
    return _exp(T);
}

float simd_transmittance(const vec4f_t _o, const vec4f_t _n, const float s, const gaussians_t &gaussians)
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

float transmittance_step(const vec4f_t o, const vec4f_t n, const float s, const float delta, const std::vector<gaussian_t> gaussians)
{
    float T = 0.f;
    for (float t = 0; t <= s; t += delta)
    {
        for (const gaussian_t &g : gaussians)
            T += delta * g.pdf(o + n * t);
    }
    return _exp(-T);
}

float density(const vec4f_t pt, const std::vector<gaussian_t> gaussians)
{
    float D = 0.f;
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
        const float lambda_q = G_q.sigma;
        float inner = 0.f;
        for (i8 k = -4; k <= 0; ++k)
        {
            const float s = (G_q.mu - o).dot(n) + k * lambda_q;
            float T;
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
            const simd::Vec<simd::Float> s = (G_q.mu - o).dot(n) + simd::set1((float)k) * lambda_q;
            simd::Vec<simd::Float> T;
            T = broadcast_transmittance(o, n, s, gaussians);
            inner += G_q.pdf(o + (n * s)) * T * lambda_q;
        }
        L_hat = L_hat + (G_q.albedo * inner);
    }
    return L_hat;
}
