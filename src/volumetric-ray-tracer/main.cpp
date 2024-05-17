#include <cmath>
#include <cstdlib>
#include <fmt/core.h>
#include <thread>
#include <vk-renderer/vk-renderer.h>
#include <include/error_fmt.h>
#include <sys/wait.h>
#include <sched.h>

#include <include/tsimd_sh.H>

#ifdef __AVX512F__
#define SIMD_BYTES 64
#else
#define SIMD_BYTES 32
#endif
#define SIMD_FLOATS (SIMD_BYTES/sizeof(float))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <include/stb_image_write.h>

#pragma GCC diagnostic pop

#include "types.h"

// TODO: Transfer erf approximations to simd and add timing analysis

/// generated using spline interpolation from -3.1 to 3.1 in steps of .6
static float spline_erf(const float x)
{
    if (x < -2.9f) return -1.f;
    else if (-2.9f <= x && x < -2.3f) { return 0.00019103826f * (x - -2.9f) * (x - -2.9f) * (x - -2.9f) + 0.00034386886f * (x - -2.9f) * (x - -2.9f) + 0.0002048055f * (x - -2.9f) + -0.9999589f; }
    else if (-2.3f <= x && x < -1.7f) { return 0.0039601973f * (x - -2.3f) * (x - -2.3f) * (x - -2.3f) + 0.007472224f * (x - -2.3f) * (x - -2.3f) + 0.0048944615f * (x - -2.3f) + -0.99885684f; }
    else if (-1.7f <= x && x < -1.1f) { return 0.043702256f * (x - -1.7f) * (x - -1.7f) * (x - -1.7f) + 0.08613629f * (x - -1.7f) * (x - -1.7f) + 0.061059568f * (x - -1.7f) + -0.98379046f; }
    else if (-1.1f <= x && x < -0.5f) { return 0.1663916f * (x - -1.1f) * (x - -1.1f) * (x - -1.1f) + 0.38564116f * (x - -1.1f) * (x - -1.1f) + 0.34412605f * (x - -1.1f) + -0.8802051f; }
    else if (-0.5f <= x && x < 0.1f) { return 0.066660866f * (x - -0.5f) * (x - -0.5f) * (x - -0.5f) + 0.50563073f * (x - -0.5f) * (x - -0.5f) + 0.8788892f * (x - -0.5f) + -0.5204999f; }
    else if (0.1f <= x && x < 0.7f) { return -0.3536934f * (x - 0.1f) * (x - 0.1f) * (x - 0.1f) + -0.1310174f * (x - 0.1f) * (x - 0.1f) + 1.1036571f * (x - 0.1f) + 0.112462915f; }
    else if (0.7f <= x && x < 1.3f) { return -0.2300452f * (x - 0.7f) * (x - 0.7f) * (x - 0.7f) + -0.5450987f * (x - 0.7f) * (x - 0.7f) + 0.6979875f * (x - 0.7f) + 0.6778012f; }
    else if (1.3f <= x && x < 1.9f) { return 0.15578617f * (x - 1.3f) * (x - 1.3f) * (x - 1.3f) + -0.26468363f * (x - 1.3f) * (x - 1.3f) + 0.21211804f * (x - 1.3f) + 0.93400794f; }
    else if (1.9f <= x && x < 2.5f) { return 0.12406375f * (x - 1.9f) * (x - 1.9f) * (x - 1.9f) + -0.041368887f * (x - 1.9f) * (x - 1.9f) + 0.028486524f * (x - 1.9f) + 0.9927904f; }
    else if (2.5f <= x && x < 3.1f) { return 0.02131252f * (x - 2.5f) * (x - 2.5f) * (x - 2.5f) + -0.0030063519f * (x - 2.5f) * (x - 2.5f) + 0.0018613797f * (x - 2.5f) + 0.999593f; }
    return 1.f;
}

static simd::Vec<simd::Float> simd_spline_erf(const simd::Vec<simd::Float> x)
{
    simd::Vec<simd::Float> value = simd::set1(1.f);
    value = simd::ifelse(x < simd::set1(-2.9f), simd::set1(-1.f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-2.9f) <= x, x < simd::set1(-2.3f)), simd::set1(0.00019103826f) * (x - simd::set1(-2.9f)) * (x - simd::set1(-2.9f)) * (x - simd::set1(-2.9f)) + simd::set1(0.00034386886f) * (x - simd::set1(-2.9f)) * (x - simd::set1(-2.9f)) + simd::set1(0.0002048055f) * (x - simd::set1(-2.9f)) + simd::set1(-0.9999589f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-2.3f) <= x, x < simd::set1(-1.7f)), simd::set1(0.0039601973f) * (x - simd::set1(-2.3f)) * (x - simd::set1(-2.3f)) * (x - simd::set1(-2.3f)) + simd::set1(0.007472224f) * (x - simd::set1(-2.3f)) * (x - simd::set1(-2.3f)) + simd::set1(0.0048944615f) * (x - simd::set1(-2.3f)) + simd::set1(-0.99885684f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-1.7f) <= x, x < simd::set1(-1.1f)), simd::set1(0.043702256f) * (x - simd::set1(-1.7f)) * (x - simd::set1(-1.7f)) * (x - simd::set1(-1.7f)) + simd::set1(0.08613629f) * (x - simd::set1(-1.7f)) * (x - simd::set1(-1.7f)) + simd::set1(0.061059568f) * (x - simd::set1(-1.7f)) + simd::set1(-0.98379046f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-1.1f) <= x, x < simd::set1(-0.5f)), simd::set1(0.1663916f) * (x - simd::set1(-1.1f)) * (x - simd::set1(-1.1f)) * (x - simd::set1(-1.1f)) + simd::set1(0.38564116f) * (x - simd::set1(-1.1f)) * (x - simd::set1(-1.1f)) + simd::set1(0.34412605f) * (x - simd::set1(-1.1f)) + simd::set1(-0.8802051f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-0.5f) <= x, x < simd::set1(0.1f)),  simd::set1(0.066660866f) * (x - simd::set1(-0.5f)) * (x - simd::set1(-0.5f)) * (x - simd::set1(-0.5f)) + simd::set1(0.50563073f) * (x - simd::set1(-0.5f)) * (x - simd::set1(-0.5f)) + simd::set1(0.8788892f) * (x - simd::set1(-0.5f)) + simd::set1(-0.5204999f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(0.1f) <= x, x < simd::set1(0.7f)),   simd::set1(-0.3536934f) * (x - simd::set1(0.1f)) * (x - simd::set1(0.1f)) * (x - simd::set1(0.1f)) + simd::set1(-0.1310174f) * (x - simd::set1(0.1f)) * (x - simd::set1(0.1f)) + simd::set1(1.1036571f) * (x - simd::set1(0.1f)) + simd::set1(0.112462915f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(0.7f) <= x, x < simd::set1(1.3f)),   simd::set1(-0.2300452f) * (x - simd::set1(0.7f)) * (x - simd::set1(0.7f)) * (x - simd::set1(0.7f)) + simd::set1(-0.5450987f) * (x - simd::set1(0.7f)) * (x - simd::set1(0.7f)) + simd::set1(0.6979875f) * (x - simd::set1(0.7f)) + simd::set1(0.6778012f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(1.3f) <= x, x < simd::set1(1.9f)),   simd::set1(0.15578617f) * (x - simd::set1(1.3f)) * (x - simd::set1(1.3f)) * (x - simd::set1(1.3f)) + simd::set1(-0.26468363f) * (x - simd::set1(1.3f)) * (x - simd::set1(1.3f)) + simd::set1(0.21211804f) * (x - simd::set1(1.3f)) + simd::set1(0.93400794f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(1.9f) <= x, x < simd::set1(2.5f)),   simd::set1(0.12406375f) * (x - simd::set1(1.9f)) * (x - simd::set1(1.9f)) * (x - simd::set1(1.9f)) + simd::set1(-0.041368887f) * (x - simd::set1(1.9f)) * (x - simd::set1(1.9f)) + simd::set1(0.028486524f) * (x - simd::set1(1.9f)) + simd::set1(0.9927904f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(2.5f) <= x, x < simd::set1(3.1f)),   simd::set1(0.02131252f) * (x - simd::set1(2.5f)) * (x - simd::set1(2.5f)) * (x - simd::set1(2.5f)) + simd::set1(-0.0030063519f) * (x - simd::set1(2.5f)) * (x - simd::set1(2.5f)) + simd::set1(0.0018613797f) * (x - simd::set1(2.5f)) + simd::set1(0.999593f), value);
    return value;
}

/// use half of the spline approximation and mirror it w.r.t. the origin
/// might be valid to use in case of simd since all if cases have to be considered regardless
static float spline_erf_mirror(float x)
{
    const float sign = x < 0 ? 1.f : -1.f;
    float value = -1.f;
    x = x > 0 ? -x : x; // take the negative absolute value
    if (-3.3f <= x && x <= -2.6f) { value = -0.000112409376f * (x - -3.3f) * (x - -3.3f) * (x - -3.3f) + -0.00023605968f * (x - -3.3f) * (x - -3.3f) + -0.00010581506f * (x - -3.3f) + -0.99999696f; }
    else if (-2.6f <= x && x <= -1.9f) { value = 0.0012324096f * (x - -2.6f) * (x - -2.6f) * (x - -2.6f) + 0.0023520004f * (x - -2.6f) * (x - -2.6f) + 0.0013753435f * (x - -2.6f) + -0.99976397f; }
    else if (-1.9f <= x && x <= -1.2f) { value = 0.014164185f * (x - -1.9f) * (x - -1.9f) * (x - -1.9f) + 0.032096792f * (x - -1.9f) * (x - -1.9f) + 0.025489498f * (x - -1.9f) + -0.9927904f; }
    else if (-1.2f <= x && x <= -0.5f) { value = 0.14258419f * (x - -1.2f) * (x - -1.2f) * (x - -1.2f) + 0.33152357f * (x - -1.2f) * (x - -1.2f) + 0.28002375f * (x - -1.2f) + -0.91031396f; }
    else if (-0.5f <= x && x <= 0.f) { value = 0.09140208f * (x - -0.5f) * (x - -0.5f) * (x - -0.5f) + 0.52346796f * (x - -0.5f) * (x - -0.5f) + 0.87851787f * (x - -0.5f) + -0.5204999f; }
    return value * sign;
}

static simd::Vec<simd::Float> simd_spline_erf_mirror(simd::Vec<simd::Float> x)
{
    const simd::Vec<simd::Float> sign = simd::ifelse(x < simd::set1(0.f), simd::set1(1.f), simd::set1(-1.f));
    simd::Vec<simd::Float> value = simd::set1(-1.f);
    x = simd::ifelse(x > simd::set1(0.f), -x, x);
    value = simd::ifelse(simd::bit_and(simd::set1(-2.9f) <= x, x < simd::set1(-2.3f)), simd::set1(0.00019103826f) * (x - simd::set1(-2.9f)) * (x - simd::set1(-2.9f)) * (x - simd::set1(-2.9f)) + simd::set1(0.00034386886f) * (x - simd::set1(-2.9f)) * (x - simd::set1(-2.9f)) + simd::set1(0.0002048055f) * (x - simd::set1(-2.9f)) + simd::set1(-0.9999589f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-2.3f) <= x, x < simd::set1(-1.7f)), simd::set1(0.0039601973f) * (x - simd::set1(-2.3f)) * (x - simd::set1(-2.3f)) * (x - simd::set1(-2.3f)) + simd::set1(0.007472224f) * (x - simd::set1(-2.3f)) * (x - simd::set1(-2.3f)) + simd::set1(0.0048944615f) * (x - simd::set1(-2.3f)) + simd::set1(-0.99885684f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-1.7f) <= x, x < simd::set1(-1.1f)), simd::set1(0.043702256f) * (x - simd::set1(-1.7f)) * (x - simd::set1(-1.7f)) * (x - simd::set1(-1.7f)) + simd::set1(0.08613629f) * (x - simd::set1(-1.7f)) * (x - simd::set1(-1.7f)) + simd::set1(0.061059568f) * (x - simd::set1(-1.7f)) + simd::set1(-0.98379046f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-1.1f) <= x, x < simd::set1(-0.5f)), simd::set1(0.1663916f) * (x - simd::set1(-1.1f)) * (x - simd::set1(-1.1f)) * (x - simd::set1(-1.1f)) + simd::set1(0.38564116f) * (x - simd::set1(-1.1f)) * (x - simd::set1(-1.1f)) + simd::set1(0.34412605f) * (x - simd::set1(-1.1f)) + simd::set1(-0.8802051f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-0.5f) <= x, x <= simd::set1(0.0f)),  simd::set1(0.066660866f) * (x - simd::set1(-0.5f)) * (x - simd::set1(-0.5f)) * (x - simd::set1(-0.5f)) + simd::set1(0.50563073f) * (x - simd::set1(-0.5f)) * (x - simd::set1(-0.5f)) + simd::set1(0.8788892f) * (x - simd::set1(-0.5f)) + simd::set1(-0.5204999f), value);
    return value * sign;
}

static float taylor_erf(const float x)
{
    if (x <= -2.f) return -1.f;
    if (x >= 2.f) return 1.f;
    return 2/std::sqrt(M_PIf) * (x + 1/3.f * -std::pow(x, 3.f) + 1/10.f * std::pow(x, 5.f) + 1/42.f * -std::pow(x, 7.f));
}

/// generated using spline interpolation with supports [-10.0 -9.0 -8.0 -7.0 -6.0 -5.0 -4.5 -4.0 -3.5 -3.0 -2.5 -2.0 -1.75 -1.5 -1.25 -1.0 -0.75 -0.5 -0.25 0.0]
static float spline_exp(float x)
{
    if (x <= -9.0f) return -1.f;
    else if (-9.0f <= x && x <= -8.0f) { return 2.0944866e-5f * (x - -9.0f) * (x - -9.0f) * (x - -9.0f) + 6.2834595e-5f * (x - -9.0f) * (x - -9.0f) + 0.0001198996f * (x - -9.0f) + 0.0001234098f; }
    else if (-8.0f <= x && x <= -7.0f) { return 2.9318619e-5f * (x - -8.0f) * (x - -8.0f) * (x - -8.0f) + 0.00015079045f * (x - -8.0f) * (x - -8.0f) + 0.00033352466f * (x - -8.0f) + 0.00033546262f; }
    else if (-7.0f <= x && x <= -6.0f) { return 9.210422e-5f * (x - -7.0f) * (x - -7.0f) * (x - -7.0f) + 0.0004271031f * (x - -7.0f) * (x - -7.0f) + 0.00091141823f * (x - -7.0f) + 0.000911882f; }
    else if (-6.0f <= x && x <= -5.0f) { return 0.00022834886f * (x - -6.0f) * (x - -6.0f) * (x - -6.0f) + 0.0011121497f * (x - -6.0f) * (x - -6.0f) + 0.002450671f * (x - -6.0f) + 0.0024787523f; }
    else if (-5.0f <= x && x <= -4.5f) { return 0.0006963741f * (x - -5.0f) * (x - -5.0f) * (x - -5.0f) + 0.0032012719f * (x - -5.0f) * (x - -5.0f) + 0.0067640925f * (x - -5.0f) + 0.006737947f; }
    else if (-4.5f <= x && x <= -4.0f) { return 0.0015094817f * (x - -4.5f) * (x - -4.5f) * (x - -4.5f) + 0.0054654945f * (x - -4.5f) * (x - -4.5f) + 0.011097476f * (x - -4.5f) + 0.011108996f; }
    else if (-4.0f <= x && x <= -3.5f) { return 0.0023322464f * (x - -4.0f) * (x - -4.0f) * (x - -4.0f) + 0.008963864f * (x - -4.0f) * (x - -4.0f) + 0.018312154f * (x - -4.0f) + 0.01831564f; }
    else if (-3.5f <= x && x <= -3.0f) { return 0.0038776079f * (x - -3.5f) * (x - -3.5f) * (x - -3.5f) + 0.0147802755f * (x - -3.5f) * (x - -3.5f) + 0.030184224f * (x - -3.5f) + 0.030197384f; }
    else if (-3.0f <= x && x <= -2.5f) { return 0.006420028f * (x - -3.0f) * (x - -3.0f) * (x - -3.0f) + 0.024410319f * (x - -3.0f) * (x - -3.0f) + 0.049779523f * (x - -3.0f) + 0.049787067f; }
    else if (-2.5f <= x && x <= -2.0f) { return 0.010444719f * (x - -2.5f) * (x - -2.5f) * (x - -2.5f) + 0.040077396f * (x - -2.5f) * (x - -2.5f) + 0.08202338f * (x - -2.5f) + 0.082085f; }
    else if (-2.0f <= x && x <= -1.75f) { return 0.017753968f * (x - -2.0f) * (x - -2.0f) * (x - -2.0f) + 0.06670835f * (x - -2.0f) * (x - -2.0f) + 0.13541625f * (x - -2.0f) + 0.13533528f; }
    else if (-1.75f <= x && x <= -1.5f) { return 0.026580833f * (x - -1.75f) * (x - -1.75f) * (x - -1.75f) + 0.08664397f * (x - -1.75f) * (x - -1.75f) + 0.17375433f * (x - -1.75f) + 0.17377394f; }
    else if (-1.5f <= x && x <= -1.25f) { return 0.03215266f * (x - -1.5f) * (x - -1.5f) * (x - -1.5f) + 0.11075847f * (x - -1.5f) * (x - -1.5f) + 0.22310494f * (x - -1.5f) + 0.22313017f; }
    else if (-1.25f <= x && x <= -1.0f) { return 0.04326379f * (x - -1.25f) * (x - -1.25f) * (x - -1.25f) + 0.14320631f * (x - -1.25f) * (x - -1.25f) + 0.28659615f * (x - -1.25f) + 0.2865048f; }
    else if (-1.0f <= x && x <= -0.75f) { return 0.04961379f * (x - -1.0f) * (x - -1.0f) * (x - -1.0f) + 0.18041666f * (x - -1.0f) * (x - -1.0f) + 0.36750188f * (x - -1.0f) + 0.36787945f; }
    else if (-0.75f <= x && x <= -0.5f) { return 0.08547847f * (x - -0.75f) * (x - -0.75f) * (x - -0.75f) + 0.2445255f * (x - -0.75f) * (x - -0.75f) + 0.47373742f * (x - -0.75f) + 0.47236654f; }
    else if (-0.5f <= x && x <= -0.25f) { return 0.02860214f * (x - -0.5f) * (x - -0.5f) * (x - -0.5f) + 0.2659771f * (x - -0.5f) * (x - -0.5f) + 0.60136306f * (x - -0.5f) + 0.60653067f; }
    else if (-0.25f <= x && x <= 0.0f) { return 0.3395703f * (x - -0.25f) * (x - -0.25f) * (x - -0.25f) + 0.52065486f * (x - -0.25f) * (x - -0.25f) + 0.7980211f * (x - -0.25f) + 0.7788008f; }
    return 1.f;
}

static float (*_erf)(float) = std::erf;
static float (*_exp)(float) = std::exp;

static FILE *LOG = NULL;

static float transmittance(const vec4f_t o, const vec4f_t n, const float s, const std::vector<gaussian_t> gaussians)
{
    float T = 0.f;
    for (const gaussian_t &g_q : gaussians)
    {
        const float mu_bar = (g_q.mu - o).dot(n);
        const float c_bar = g_q.magnitude * _exp(-((g_q.mu - o).dot(g_q.mu - o) - std::pow(mu_bar, 2.f))/(2*std::pow(g_q.sigma, 2.f)));
        if (LOG) fmt::print(LOG, "{}, ", -((g_q.mu - o).dot(g_q.mu - o) - std::pow(mu_bar, 2.f))/(2*std::pow(g_q.sigma, 2.f)));
        T += ((g_q.sigma * c_bar)/(std::sqrt(2/M_PIf))) * (_erf(-mu_bar/(std::sqrt(2) * g_q.sigma)) - _erf((s - mu_bar)/(std::sqrt(2) * g_q.sigma)));
    }
    if (LOG) fmt::println(LOG, "{}", T);
    return _exp(T);
}

static float transmittance_step(const vec4f_t o, const vec4f_t n, const float s, const float delta, const std::vector<gaussian_t> gaussians)
{
    float T = 0.f;
    for (float t = 0; t <= s; t += delta)
    {
        for (const gaussian_t &g : gaussians)
            T += delta * g.pdf(o + n * t);
    }
    return _exp(-T);
}

static float density(const vec4f_t pt, const std::vector<gaussian_t> gaussians)
{
    float D = 0.f;
    for (const gaussian_t &g : gaussians)
    {
        D += g.pdf(pt);
    }
    return D;
}

static bool approx_transmittance = false;
static bool new_approx_transmittance = approx_transmittance;

static vec4f_t l_hat(const vec4f_t o, const vec4f_t n, const std::vector<gaussian_t> gaussians)
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
            if (approx_transmittance)
                T = transmittance_step(o, n, s, lambda_q, gaussians);
            else
                T = transmittance(o, n, s, gaussians);
            inner += G_q.pdf(o + (n * s)) * T * lambda_q;
        }
        L_hat = L_hat + (G_q.albedo * inner);
    }
    return L_hat;
}

int main(i32 argc, char **argv)
{
    u64 width = 256, height = 256;
    if (argc >= 3)
    {
        width = strtoul(argv[1], NULL, 10);
        if (width == 0)
        {
            fmt::println(stderr, "[ {} ]\tInvaid width set! Setting widht to 256.", ERROR_FMT("ERROR"));
            width = 256;
        }
        height = strtoul(argv[2], NULL, 10);
        if (height == 0)
        {
            fmt::println(stderr, "[ {} ]\tInvaid height set! Setting height to 256.", ERROR_FMT("ERROR"), strerror(errno));
            height = 256;
        }
    }
    u64 new_width = width, new_height = height;
    
    std::vector<char*> args(argc);
    std::memcpy(args.data(), argv, sizeof(*argv) * argc);
    bool grid = false;
    bool with_tests = true;
    for (char *s : args)
    {
        if (std::strncmp(s, "--grid", 7) == 0) grid = true;
        else if (std::strncmp(s, "--no-tests", 11) == 0) with_tests = false;
    }

    std::vector<gaussian_t> gaussians;
    if (grid)
    {
        for (u8 i = 0; i < 4; ++i)
            for (u8 j = 0; j < 4; ++j)
                gaussians.push_back(gaussian_t{
                        .albedo{ 1.f, 0.f, 0.f, 1.f },
                        .mu{ -1.f + i * 1.f/2.f, -1.f + j * 1.f/2.f, 0.f },
                        .sigma = 1.f/8.f,
                        .magnitude = 1.f
                        });
    }
    else
    {
        gaussians = { gaussian_t{ .albedo{ 0.f, 1.f, 0.f, .1f }, .mu{ .3f, .3f, .5f }, .sigma = 0.1f, .magnitude = 2.f }, gaussian_t{ .albedo{ 0.f, 0.f, 1.f, .7f }, .mu{ -.3f, -.3f, 0.f }, .sigma = 0.4f, .magnitude = .7f }, gaussian_t{ .albedo{ 1.f, 0.f, 0.f, 1.f }, .mu{ 0.f, 0.f, 2.f }, .sigma = .75f, .magnitude = 1.f } };
    }
    std::vector<gaussian_t> staging_gaussians = gaussians;
    const vec4f_t origin = { 0.f, 0.f, -5.f };
    
    if (with_tests)
    {
        cpu_set_t mask;
        CPU_ZERO(&mask);

        if (fork() == 0)
        {
            // bind process to CPU2
            CPU_SET(2, &mask);
            sched_setaffinity(0, sizeof(mask), &mask);

            vec4f_t dir = {0.f, 0.f, 1.f};        
            FILE *CSV = std::fopen("csv/data.csv", "wd");
            if (!CSV) exit(EXIT_FAILURE);
            fmt::println(CSV, "s, T, T_s, err, D");
            for (float k = -6.f; k <= 6; k += .1f)
            {
                float s = (gaussians[2].mu - origin).dot(dir) + k * gaussians[2].sigma;
                float T = transmittance(origin, dir, s, gaussians);
                float T_s = transmittance_step(origin, dir, s, gaussians[2].sigma, gaussians);
                float err = std::abs(T - T_s);
                float D = density(origin + dir * s, gaussians);
                fmt::println(CSV, "{}, {}, {}, {}, {}", s, T, T_s, err, D);
            }
            std::fclose(CSV);
            char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/transmittance.jl", NULL };
            execvp("./julia/wrapper.sh", args);
            fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (fork() == 0)
        {
            // bind process to CPU4
            CPU_SET(4, &mask);
            sched_setaffinity(0, sizeof(mask), &mask);

            FILE *CSV = std::fopen("csv/timing_erf.csv", "wd");
            if (!CSV) exit(EXIT_FAILURE);
            fmt::println(CSV, "count, t_spline, t_mirror, t_taylor, t_std");
            std::vector<gaussian_t> growing;
            for (u8 i = 0; i < 16; ++i)
            {
                for (u8 j = 0; j < 16; ++j)
                {
                    growing.push_back(gaussian_t{
                            .albedo{ 1.f, 0.f, 0.f, 1.f },
                            .mu{ -1.f + i * 1.f/8.f, -1.f + j * 1.f/8.f, 0.f },
                            .sigma = 1.f/8.f,
                            .magnitude = 1.f
                            });
                    _erf = spline_erf;
                    auto start_time = std::chrono::high_resolution_clock::now();
                    l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
                    auto end_time = std::chrono::high_resolution_clock::now();
                    float t_spline = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
                    
                    _erf = spline_erf_mirror;
                    start_time = std::chrono::high_resolution_clock::now();
                    l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
                    end_time = std::chrono::high_resolution_clock::now();
                    float t_mirror = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

                    _erf = taylor_erf;
                    start_time = std::chrono::high_resolution_clock::now();
                    l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
                    end_time = std::chrono::high_resolution_clock::now();
                    float t_taylor = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

                    _erf = std::erf;
                    start_time = std::chrono::high_resolution_clock::now();
                    l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
                    end_time = std::chrono::high_resolution_clock::now();
                    float t_std = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

                    fmt::println(CSV, "{}, {}, {}, {}, {}", i * 16 + j + 1, t_spline, t_mirror, t_taylor, t_std);
                }
            }
            std::fclose(CSV);
            
            CSV = std::fopen("csv/timing_exp.csv", "wd");
            if (!CSV) exit(EXIT_FAILURE);
            fmt::println(CSV, "count, t_spline, t_std");
            growing.clear();
            _erf = std::erf;
            for (u8 i = 0; i < 16; ++i)
            {
                for (u8 j = 0; j < 16; ++j)
                {
                    growing.push_back(gaussian_t{
                            .albedo{ 1.f, 0.f, 0.f, 1.f },
                            .mu{ -1.f + i * 1.f/8.f, -1.f + j * 1.f/8.f, 0.f },
                            .sigma = 1.f/8.f,
                            .magnitude = 1.f
                            });
                    _exp = spline_exp;
                    auto start_time = std::chrono::high_resolution_clock::now();
                    l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
                    auto end_time = std::chrono::high_resolution_clock::now();
                    float t_spline = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

                    _exp = std::exp;
                    start_time = std::chrono::high_resolution_clock::now();
                    l_hat(origin, vec4f_t{ 0.f, 0.f, 1.f }, growing);
                    end_time = std::chrono::high_resolution_clock::now();
                    float t_std = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

                    fmt::println(CSV, "{}, {}, {}", i * 16 + j + 1, t_spline, t_std);
                }
            }
            std::fclose(CSV);
            char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/timing.jl", NULL };
            execvp("./julia/wrapper.sh", args);
            fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (fork() == 0)
        {
            // bind process to CPU6
            CPU_SET(6, &mask);
            sched_setaffinity(0, sizeof(mask), &mask);
            FILE *CSV = std::fopen("csv/erf.csv", "wd");
            if (!CSV) exit(EXIT_FAILURE);
            fmt::println(CSV, "x, spline, spline_mirror, taylor, erf");
            for (float x = -6.f; x <= 6.f; x+=0.1f)
            {
                fmt::println(CSV, "{}, {}, {}, {}, {}", x, spline_erf(x), spline_erf_mirror(x), taylor_erf(x), std::erf(x));
            }
            fclose(CSV);
            char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/cmp_erf.jl", NULL };
            execvp("./julia/wrapper.sh", args);
            fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (fork() == 0)
        {
            // bind process to CPU8
            CPU_SET(8, &mask);
            sched_setaffinity(0, sizeof(mask), &mask);
            FILE *CSV = std::fopen("csv/simd_erf.csv", "wd");
            fmt::println(CSV, "count, t_simd, t_simd_mirror, t_spline, t_std");
            for (u64 i = 1; i < 100; ++i)
            {
                float t[SIMD_FLOATS * i];
                float s[SIMD_FLOATS * i];
                for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                    t[j] = -4.f + j * 8.f/(SIMD_FLOATS * i);

                auto start_time = std::chrono::high_resolution_clock::now();
                for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                    simd::storeu(s + j, simd_spline_erf(simd::loadu(t + j)));
                auto end_time = std::chrono::high_resolution_clock::now();
                float t_simd = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
                
                start_time = std::chrono::high_resolution_clock::now();
                for (u64 j = 0; j < SIMD_FLOATS * i; j+=SIMD_FLOATS)
                    simd::storeu(s + j, simd_spline_erf_mirror(simd::loadu(t + j)));
                end_time = std::chrono::high_resolution_clock::now();
                float t_simd_mirror = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
                
                start_time = std::chrono::high_resolution_clock::now();
                for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                    s[j] = spline_erf(t[j]);
                end_time = std::chrono::high_resolution_clock::now();
                float t_spline = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
                
                start_time = std::chrono::high_resolution_clock::now();
                for (u64 j = 0; j < SIMD_FLOATS * i; ++j)
                    s[j] = std::erf(t[j]);
                end_time = std::chrono::high_resolution_clock::now();
                float t_std = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

                fmt::println(CSV, "{}, {}, {}, {}, {}", SIMD_FLOATS * i, t_simd, t_simd_mirror, t_spline, t_std);
            }
            std::fclose(CSV);
            char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/simd_erf_timing.jl", NULL };
            execvp("./julia/wrapper.sh", args);
            fmt::println(stderr, "[ {} ]\tGenerating Plots failed with error: {}", ERROR_FMT("ERROR"), strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    
    renderer_t renderer;
    float draw_time = 0.f;
    bool use_spline_approx = false;
    bool use_mirror_approx = false;
    bool use_taylor_approx = false;
    if (!renderer.init(width, height, "Test")) return EXIT_FAILURE;
    renderer.custom_imgui = [&](){
        ImGui::Begin("Gaussians");
        for (gaussian_t &g : staging_gaussians) g.imgui_controls();
        ImGui::End();
        ImGui::Begin("Debug");
        ImGui::Text("Draw Time: %f ms", draw_time);
        ImGui::Checkbox("spline", &use_spline_approx);
        ImGui::Checkbox("mirror", &use_mirror_approx);
        ImGui::Checkbox("taylor", &use_taylor_approx);
        ImGui::Checkbox("approx", &new_approx_transmittance);
        ImGui::End();
    };
    bool running = true;
    std::thread render_thread([&](){ renderer.run(running); });

    u32 *image = (u32*)std::calloc(width * height, sizeof(u32));

    bool first_frame = true;

    while (running)
    {
        if (new_width != width || new_height != height)
        {
            width = new_width;
            height = new_height;
            image = (u32*)std::realloc(image, width * height * sizeof(u32));
        }
        gaussians = staging_gaussians;
        approx_transmittance = new_approx_transmittance;
        if (use_spline_approx) _erf = spline_erf;
        else if (use_mirror_approx) _erf = spline_erf_mirror;
        else if (use_taylor_approx) _erf = taylor_erf;
        else _erf = std::erf;

        if (first_frame)
        {
            LOG = std::fopen("csv/expvals.csv", "wd");
        }

        auto start_time = std::chrono::system_clock::now();
        vec4f_t pt = { -1.f, -1.f, 0.f };
        for (u64 y = 0; y < height; ++y)
        {
            for (u64 x = 0; x < width; ++x)
            {
                vec4f_t dir = pt - origin;
                dir.normalize();
                vec4f_t color = l_hat(origin, dir, gaussians);
                u32 A = 0xFF000000; // final alpha channel is always 1
                u32 R = (u32)(color.x * 255);
                u32 G = (u32)(color.y * 255);
                u32 B = (u32)(color.z * 255);
                image[y * width + x] = A | R << 16 | G << 8 | B;
                pt.x += 1/(width/2.f);
                if (!running) goto cleanup;
            }
            pt.x = -1.f;
            pt.y += 1/(height/2.f);
        }

        auto end_time = std::chrono::system_clock::now();
        draw_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        if (first_frame)
        {
            std::fclose(LOG);
            LOG = NULL;
            first_frame = false;
        }

        if (argc >= 4)
            stbi_write_png(argv[3], width, height, 4, image, width * 4);
        renderer.stage_image(image, width, height);
    }

cleanup:
    render_thread.join();
    free(image);

    return EXIT_SUCCESS;
}
