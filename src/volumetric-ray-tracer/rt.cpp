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

float simd_transmittance(const vec4f_t o, const vec4f_t n, const float s, const std::vector<gaussian_t> gaussians)
{
    simd::Vec<simd::Float> T = simd::set1(1.f);
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
            T = transmittance(o, n, s, gaussians);
            inner += G_q.pdf(o + (n * s)) * T * lambda_q;
        }
        L_hat = L_hat + (G_q.albedo * inner);
    }
    return L_hat;
}
