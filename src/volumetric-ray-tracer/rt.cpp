#include "rt.h"
#include <include/definitions.h>

float transmittance(const vec4f_t o, const vec4f_t n, const float s, const std::vector<gaussian_t> gaussians)
{
    float T = 0.f;
    for (const gaussian_t &g_q : gaussians)
    {
        const float mu_bar = (g_q.mu - o).dot(n);
        const float c_bar = g_q.magnitude * _exp(-((g_q.mu - o).dot(g_q.mu - o) - std::pow(mu_bar, 2.f))/(2*std::pow(g_q.sigma, 2.f)));
        T += ((g_q.sigma * c_bar)/(std::sqrt(2/M_PIf))) * (_erf(-mu_bar/(std::sqrt(2) * g_q.sigma)) - _erf((s - mu_bar)/(std::sqrt(2) * g_q.sigma)));
    }
    return _exp(T);
}

float simd_transmittance(const vec4f_t _o, const vec4f_t _n, const float s, const std::vector<gaussian_t> gaussians)
{
    simd::Vec<simd::Float> T = simd::set1(0.f);
    // NOTE: this seems non-optimal, maybe use struct of arrays for gaussians and vec4s instead?
    u64 size = ((gaussians.size()/SIMD_FLOATS) + 1) * SIMD_FLOATS;
    float _mu_x[size], _mu_y[size], _mu_z[size], _magnitude[size], _sigma[size];
    for (u64 j = 0; j < size; ++j)
    {
        if (j >= gaussians.size())
        {
            _mu_x[j]      = 0.f;
            _mu_y[j]      = 0.f;
            _mu_z[j]      = 0.f;
            _magnitude[j] = 0.f;
            _sigma[j]     = 1.f;
            continue;
        }
        _mu_x[j]      = gaussians[j].mu.x;
        _mu_y[j]      = gaussians[j].mu.y;
        _mu_z[j]      = gaussians[j].mu.z;
        _magnitude[j] = gaussians[j].magnitude;
        _sigma[j]     = gaussians[j].sigma;
    }
    for (u64 i = 0; i < gaussians.size(); i += SIMD_FLOATS)
    {
        simd_gaussian_t g_q{
            .albedo{},
            .mu{ .x = simd::loadu(_mu_x), .y = simd::loadu(_mu_y), .z = simd::loadu(_mu_z) },
            .sigma = simd::loadu(_sigma),
            .magnitude = simd::loadu(_magnitude)
        };
        simd_vec4f_t o = simd_vec4f_t::from_vec4f_t(_o);
        simd_vec4f_t n = simd_vec4f_t::from_vec4f_t(_n);
        const simd::Vec<simd::Float> mu_bar = (g_q.mu - o).dot(n);
        const simd::Vec<simd::Float> c_bar = g_q.magnitude * _simd_exp(-((g_q.mu - o).dot(g_q.mu - o) - (mu_bar * mu_bar))/(simd::set1(2.f) * g_q.sigma * g_q.sigma));

        constexpr float SQRT_2_PI = 0.7978845608028654f;
        constexpr float SQRT_2 = 1.4142135623730951f;

        T += ((g_q.sigma * c_bar)/simd::set1(SQRT_2_PI)) * (_simd_erf(-mu_bar/(simd::set1(SQRT_2) * g_q.sigma)) - _simd_erf((simd::set1(s) - mu_bar)/(simd::set1(SQRT_2) * g_q.sigma)));
    }
    return _exp(simd::hadds(T));
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

vec4f_t l_hat(const vec4f_t o, const vec4f_t n, const std::vector<gaussian_t> gaussians)
{
    vec4f_t L_hat{ .x = 0.f, .y = 0.f, .z = 0.f };
    for (const gaussian_t &G_q : gaussians)
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
