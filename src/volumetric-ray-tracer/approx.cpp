#include "approx.h"
#include <fmt/core.h>

#define SIGN(x) ((x >= 0) - (x < 0))

/// generated using spline interpolation from -3.1 to 3.1 in steps of .6
f32 spline_erf(const f32 x)
{
    if (x < -2.9f) return -1.f;
    else if (x < -2.3f) { return 0.00019103826f * (x - -2.9f) * (x - -2.9f) * (x - -2.9f) + 0.00034386886f * (x - -2.9f) * (x - -2.9f) + 0.0002048055f * (x - -2.9f) + -0.9999589f; }
    else if (x < -1.7f) { return 0.0039601973f * (x - -2.3f) * (x - -2.3f) * (x - -2.3f) + 0.007472224f * (x - -2.3f) * (x - -2.3f) + 0.0048944615f * (x - -2.3f) + -0.99885684f; }
    else if (x < -1.1f) { return 0.043702256f * (x - -1.7f) * (x - -1.7f) * (x - -1.7f) + 0.08613629f * (x - -1.7f) * (x - -1.7f) + 0.061059568f * (x - -1.7f) + -0.98379046f; }
    else if (x < -0.5f) { return 0.1663916f * (x - -1.1f) * (x - -1.1f) * (x - -1.1f) + 0.38564116f * (x - -1.1f) * (x - -1.1f) + 0.34412605f * (x - -1.1f) + -0.8802051f; }
    else if (x < 0.1f) { return 0.066660866f * (x - -0.5f) * (x - -0.5f) * (x - -0.5f) + 0.50563073f * (x - -0.5f) * (x - -0.5f) + 0.8788892f * (x - -0.5f) + -0.5204999f; }
    else if (x < 0.7f) { return -0.3536934f * (x - 0.1f) * (x - 0.1f) * (x - 0.1f) + -0.1310174f * (x - 0.1f) * (x - 0.1f) + 1.1036571f * (x - 0.1f) + 0.112462915f; }
    else if (x < 1.3f) { return -0.2300452f * (x - 0.7f) * (x - 0.7f) * (x - 0.7f) + -0.5450987f * (x - 0.7f) * (x - 0.7f) + 0.6979875f * (x - 0.7f) + 0.6778012f; }
    else if (x < 1.9f) { return 0.15578617f * (x - 1.3f) * (x - 1.3f) * (x - 1.3f) + -0.26468363f * (x - 1.3f) * (x - 1.3f) + 0.21211804f * (x - 1.3f) + 0.93400794f; }
    else if (x < 2.5f) { return 0.12406375f * (x - 1.9f) * (x - 1.9f) * (x - 1.9f) + -0.041368887f * (x - 1.9f) * (x - 1.9f) + 0.028486524f * (x - 1.9f) + 0.9927904f; }
    else if (x < 3.1f) { return 0.02131252f * (x - 2.5f) * (x - 2.5f) * (x - 2.5f) + -0.0030063519f * (x - 2.5f) * (x - 2.5f) + 0.0018613797f * (x - 2.5f) + 0.999593f; }
    return 1.f;
}

simd::Vec<simd::Float> simd_spline_erf(const simd::Vec<simd::Float> x)
{
    simd::Vec<simd::Float> value = simd::set1(1.f);
    value = simd::ifelse(x < simd::set1(-2.9f), simd::set1(-1.f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-2.9f) <= x, x < simd::set1(-2.3f)), simd::set1(0.00019103826f) * (x - simd::set1(-2.9f)) * (x - simd::set1(-2.9f)) * (x - simd::set1(-2.9f)) + simd::set1(0.00034386886f) * (x - simd::set1(-2.9f)) * (x - simd::set1(-2.9f)) + simd::set1(0.0002048055f) * (x - simd::set1(-2.9f)) + simd::set1(-0.9999589f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-2.3f) <= x, x < simd::set1(-1.7f)), simd::set1(0.0039601973f) * (x - simd::set1(-2.3f)) * (x - simd::set1(-2.3f)) * (x - simd::set1(-2.3f)) + simd::set1(0.007472224f) * (x - simd::set1(-2.3f)) * (x - simd::set1(-2.3f)) + simd::set1(0.0048944615f) * (x - simd::set1(-2.3f)) + simd::set1(-0.99885684f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-1.7f) <= x, x < simd::set1(-1.1f)), simd::set1(0.043702256f) * (x - simd::set1(-1.7f)) * (x - simd::set1(-1.7f)) * (x - simd::set1(-1.7f)) + simd::set1(0.08613629f) * (x - simd::set1(-1.7f)) * (x - simd::set1(-1.7f)) + simd::set1(0.061059568f) * (x - simd::set1(-1.7f)) + simd::set1(-0.98379046f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-1.1f) <= x, x < simd::set1(-0.5f)), simd::set1(0.1663916f) * (x - simd::set1(-1.1f)) * (x - simd::set1(-1.1f)) * (x - simd::set1(-1.1f)) + simd::set1(0.38564116f) * (x - simd::set1(-1.1f)) * (x - simd::set1(-1.1f)) + simd::set1(0.34412605f) * (x - simd::set1(-1.1f)) + simd::set1(-0.8802051f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-0.5f) <= x, x <  simd::set1(0.1f)),  simd::set1(0.066660866f) * (x - simd::set1(-0.5f)) * (x - simd::set1(-0.5f)) * (x - simd::set1(-0.5f)) + simd::set1(0.50563073f) * (x - simd::set1(-0.5f)) * (x - simd::set1(-0.5f)) + simd::set1(0.8788892f) * (x - simd::set1(-0.5f)) + simd::set1(-0.5204999f), value);
    value = simd::ifelse(simd::bit_and( simd::set1(0.1f) <= x, x <  simd::set1(0.7f)),  simd::set1(-0.3536934f) * (x - simd::set1(0.1f)) * (x - simd::set1(0.1f)) * (x - simd::set1(0.1f)) + simd::set1(-0.1310174f) * (x - simd::set1(0.1f)) * (x - simd::set1(0.1f)) + simd::set1(1.1036571f) * (x - simd::set1(0.1f)) + simd::set1(0.112462915f), value);
    value = simd::ifelse(simd::bit_and( simd::set1(0.7f) <= x, x <  simd::set1(1.3f)),  simd::set1(-0.2300452f) * (x - simd::set1(0.7f)) * (x - simd::set1(0.7f)) * (x - simd::set1(0.7f)) + simd::set1(-0.5450987f) * (x - simd::set1(0.7f)) * (x - simd::set1(0.7f)) + simd::set1(0.6979875f) * (x - simd::set1(0.7f)) + simd::set1(0.6778012f), value);
    value = simd::ifelse(simd::bit_and( simd::set1(1.3f) <= x, x <  simd::set1(1.9f)),  simd::set1(0.15578617f) * (x - simd::set1(1.3f)) * (x - simd::set1(1.3f)) * (x - simd::set1(1.3f)) + simd::set1(-0.26468363f) * (x - simd::set1(1.3f)) * (x - simd::set1(1.3f)) + simd::set1(0.21211804f) * (x - simd::set1(1.3f)) + simd::set1(0.93400794f), value);
    value = simd::ifelse(simd::bit_and( simd::set1(1.9f) <= x, x <  simd::set1(2.5f)),  simd::set1(0.12406375f) * (x - simd::set1(1.9f)) * (x - simd::set1(1.9f)) * (x - simd::set1(1.9f)) + simd::set1(-0.041368887f) * (x - simd::set1(1.9f)) * (x - simd::set1(1.9f)) + simd::set1(0.028486524f) * (x - simd::set1(1.9f)) + simd::set1(0.9927904f), value);
    value = simd::ifelse(simd::bit_and( simd::set1(2.5f) <= x, x <  simd::set1(3.1f)),  simd::set1(0.02131252f) * (x - simd::set1(2.5f)) * (x - simd::set1(2.5f)) * (x - simd::set1(2.5f)) + simd::set1(-0.0030063519f) * (x - simd::set1(2.5f)) * (x - simd::set1(2.5f)) + simd::set1(0.0018613797f) * (x - simd::set1(2.5f)) + simd::set1(0.999593f), value);
    return value;
}

/// use half of the spline approximation and mirror it w.r.t. the origin
/// might be valid to use in case of simd since all if cases have to be considered regardless
f32 spline_erf_mirror(f32 x)
{
    const f32 sign = -SIGN(x);
    f32 value = -1.f;
    x = x > 0 ? -x : x; // take the negative absolute value
    if (-3.3f < x && x < -2.6f) { value = -0.000112409376f * (x - -3.3f) * (x - -3.3f) * (x - -3.3f) + -0.00023605968f * (x - -3.3f) * (x - -3.3f) + -0.00010581506f * (x - -3.3f) + -0.99999696f; }
    else if (-2.6f <= x && x < -1.9f) { value = 0.0012324096f * (x - -2.6f) * (x - -2.6f) * (x - -2.6f) + 0.0023520004f * (x - -2.6f) * (x - -2.6f) + 0.0013753435f * (x - -2.6f) + -0.99976397f; }
    else if (-1.9f <= x && x < -1.2f) { value = 0.014164185f * (x - -1.9f) * (x - -1.9f) * (x - -1.9f) + 0.032096792f * (x - -1.9f) * (x - -1.9f) + 0.025489498f * (x - -1.9f) + -0.9927904f; }
    else if (-1.2f <= x && x < -0.5f) { value = 0.14258419f * (x - -1.2f) * (x - -1.2f) * (x - -1.2f) + 0.33152357f * (x - -1.2f) * (x - -1.2f) + 0.28002375f * (x - -1.2f) + -0.91031396f; }
    else if (-0.5f <= x && x <= 0.f) { value = 0.09140208f * (x - -0.5f) * (x - -0.5f) * (x - -0.5f) + 0.52346796f * (x - -0.5f) * (x - -0.5f) + 0.87851787f * (x - -0.5f) + -0.5204999f; }
    return value * sign;
}

simd::Vec<simd::Float> simd_spline_erf_mirror(simd::Vec<simd::Float> x)
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

f32 taylor_erf(const f32 x)
{
    if (x <= -2.f) return -1.f;
    if (x >= 2.f) return 1.f;
    return 2/std::sqrt(M_PIf) * (x + 1/3.f * -std::pow(x, 3.f) + 1/10.f * std::pow(x, 5.f) + 1/42.f * -std::pow(x, 7.f));
}

f32 abramowitz_stegun_erf(f32 x)
{
    f32 sign = SIGN(x);
    x *= sign;
    const f32 square = x*x;
    constexpr f32 a[] = { 0.278393f, 0.230389f, 0.000972f, 0.078108f };
    const f32 denom = 1 + a[0]*x + a[1]*square + a[2]*square*x + a[3]*square*square;
    const f32 val = 1 - 1/(denom*denom*denom*denom);
    return val * sign;
}

simd::Vec<simd::Float> simd_abramowitz_stegun_erf(simd::Vec<simd::Float> x)
{
    const simd::Vec<simd::Float> sign = simd::ifelse(x < simd::set1(0.f), simd::set1(-1.f), simd::set1(1.f));
    x *= sign;
    const simd::Vec<simd::Float> square = x*x;
    constexpr f32 a[] = { 0.278393f, 0.230389f, 0.000972f, 0.078108f };
    const simd::Vec<simd::Float> denom = simd::set1(1.f) + simd::set1(a[0])*x + simd::set1(a[1])*square + simd::set1(a[2])*square*x + simd::set1(a[3])*square*square;
    const simd::Vec<simd::Float> val = simd::set1(1.f) - simd::set1(1.f)/(denom*denom*denom*denom);
    return val * sign;
}

static constexpr f32 a = (1 << 23)/std::numbers::ln2_v<f32>;
static constexpr f32 b = (1 << 23) * (127 - 0.043677448f);
static constexpr f32 c = (1 << 23);
static constexpr f32 d = (1 << 23) * 255;
f32 fast_exp(f32 x)
{
    x = a *x + b;
    if (x < c || x > d)
        x = (x < c) ? 0.f : d;

    u32 n = static_cast<u32>(x);
    std::memcpy(&x, &n, 4);
    return x;
}

simd::Vec<simd::Float> simd_fast_exp(simd::Vec<simd::Float> x)
{
    x = simd::set1(a) *x + simd::set1(b);
    x = simd::ifelse(simd::bit_or(x < simd::set1(c), x > simd::set1(d)), simd::ifelse(x < simd::set1(c), simd::set1(0.f), simd::set1(d)), x);
    return simd::reinterpret<simd::Float>(simd::cvts<simd::Int>(x));
}

/// generated using spline interpolation with supports [-10.0 -9.0 -8.0 -7.0 -6.0 -5.0 -4.5 -4.0 -3.5 -3.0 -2.5 -2.0 -1.75 -1.5 -1.25 -1.0 -0.75 -0.5 -0.25 0.0]
f32 spline_exp(f32 x)
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

/// generated using spline interpolation with supports [-10.0 -9.0 -8.0 -7.0 -6.0 -5.0 -4.5 -4.0 -3.5 -3.0 -2.5 -2.0 -1.75 -1.5 -1.25 -1.0 -0.75 -0.5 -0.25 0.0]
simd::Vec<simd::Float> simd_spline_exp(simd::Vec<simd::Float> x)
{
    simd::Vec<simd::Float> value = simd::set1(1.f);
    simd::ifelse(x <= simd::set1(-9.0f), simd::set1(-1.f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-9.0f) <= x, x <= simd::set1(-8.0f)), simd::set1(2.0944866e-5f) * (x - simd::set1(-9.0f)) * (x - simd::set1(-9.0f)) * (x - simd::set1(-9.0f)) + simd::set1(6.2834595e-5f) * (x - simd::set1(-9.0f)) * (x - simd::set1(-9.0f)) + simd::set1(0.0001198996f) * (x - simd::set1(-9.0f)) + simd::set1(0.0001234098f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-8.0f) <= x, x <= simd::set1(-7.0f)), simd::set1(2.9318619e-5f) * (x - simd::set1(-8.0f)) * (x - simd::set1(-8.0f)) * (x - simd::set1(-8.0f)) + simd::set1(0.00015079045f) * (x - simd::set1(-8.0f)) * (x - simd::set1(-8.0f)) + simd::set1(0.00033352466f) * (x - simd::set1(-8.0f)) + simd::set1(0.00033546262f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-7.0f) <= x, x <= simd::set1(-6.0f)), simd::set1(9.210422e-5f) * (x - simd::set1(-7.0f)) * (x - simd::set1(-7.0f)) * (x - simd::set1(-7.0f)) + simd::set1(0.0004271031f) * (x - simd::set1(-7.0f)) * (x - simd::set1(-7.0f)) + simd::set1(0.00091141823f) * (x - simd::set1(-7.0f)) + simd::set1(0.000911882f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-6.0f) <= x, x <= simd::set1(-5.0f)), simd::set1(0.00022834886f) * (x - simd::set1(-6.0f)) * (x - simd::set1(-6.0f)) * (x - simd::set1(-6.0f)) + simd::set1(0.0011121497f) * (x - simd::set1(-6.0f)) * (x - simd::set1(-6.0f)) + simd::set1(0.002450671f) * (x - simd::set1(-6.0f)) + simd::set1(0.0024787523f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-5.0f) <= x, x <= simd::set1(-4.5f)), simd::set1(0.0006963741f) * (x - simd::set1(-5.0f)) * (x - simd::set1(-5.0f)) * (x - simd::set1(-5.0f)) + simd::set1(0.0032012719f) * (x - simd::set1(-5.0f)) * (x - simd::set1(-5.0f)) + simd::set1(0.0067640925f) * (x - simd::set1(-5.0f)) + simd::set1(0.006737947f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-4.5f) <= x, x <= simd::set1(-4.0f)), simd::set1(0.0015094817f) * (x - simd::set1(-4.5f)) * (x - simd::set1(-4.5f)) * (x - simd::set1(-4.5f)) + simd::set1(0.0054654945f) * (x - simd::set1(-4.5f)) * (x - simd::set1(-4.5f)) + simd::set1(0.011097476f) * (x - simd::set1(-4.5f)) + simd::set1(0.011108996f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-4.0f) <= x, x <= simd::set1(-3.5f)), simd::set1(0.0023322464f) * (x - simd::set1(-4.0f)) * (x - simd::set1(-4.0f)) * (x - simd::set1(-4.0f)) + simd::set1(0.008963864f) * (x - simd::set1(-4.0f)) * (x - simd::set1(-4.0f)) + simd::set1(0.018312154f) * (x - simd::set1(-4.0f)) + simd::set1(0.01831564f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-3.5f) <= x, x <= simd::set1(-3.0f)), simd::set1(0.0038776079f) * (x - simd::set1(-3.5f)) * (x - simd::set1(-3.5f)) * (x - simd::set1(-3.5f)) + simd::set1(0.0147802755f) * (x - simd::set1(-3.5f)) * (x - simd::set1(-3.5f)) + simd::set1(0.030184224f) * (x - simd::set1(-3.5f)) + simd::set1(0.030197384f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-3.0f) <= x, x <= simd::set1(-2.5f)), simd::set1(0.006420028f) * (x - simd::set1(-3.0f)) * (x - simd::set1(-3.0f)) * (x - simd::set1(-3.0f)) + simd::set1(0.024410319f) * (x - simd::set1(-3.0f)) * (x - simd::set1(-3.0f)) + simd::set1(0.049779523f) * (x - simd::set1(-3.0f)) + simd::set1(0.049787067f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-2.5f) <= x, x <= simd::set1(-2.0f)), simd::set1(0.010444719f) * (x - simd::set1(-2.5f)) * (x - simd::set1(-2.5f)) * (x - simd::set1(-2.5f)) + simd::set1(0.040077396f) * (x - simd::set1(-2.5f)) * (x - simd::set1(-2.5f)) + simd::set1(0.08202338f) * (x - simd::set1(-2.5f)) + simd::set1(0.082085f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-2.0f) <= x, x <= simd::set1(-1.75f)), simd::set1(0.017753968f) * (x - simd::set1(-2.0f)) * (x - simd::set1(-2.0f)) * (x - simd::set1(-2.0f)) + simd::set1(0.06670835f) * (x - simd::set1(-2.0f)) * (x - simd::set1(-2.0f)) + simd::set1(0.13541625f) * (x - simd::set1(-2.0f)) + simd::set1(0.13533528f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-1.75f) <= x, x <= simd::set1(-1.5f)), simd::set1(0.026580833f) * (x - simd::set1(-1.75f)) * (x - simd::set1(-1.75f)) * (x - simd::set1(-1.75f)) + simd::set1(0.08664397f) * (x - simd::set1(-1.75f)) * (x - simd::set1(-1.75f)) + simd::set1(0.17375433f) * (x - simd::set1(-1.75f)) + simd::set1(0.17377394f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-1.5f) <= x, x <= simd::set1(-1.25f)), simd::set1(0.03215266f) * (x - simd::set1(-1.5f)) * (x - simd::set1(-1.5f)) * (x - simd::set1(-1.5f)) + simd::set1(0.11075847f) * (x - simd::set1(-1.5f)) * (x - simd::set1(-1.5f)) + simd::set1(0.22310494f) * (x - simd::set1(-1.5f)) + simd::set1(0.22313017f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-1.25f) <= x, x <= simd::set1(-1.0f)), simd::set1(0.04326379f) * (x - simd::set1(-1.25f)) * (x - simd::set1(-1.25f)) * (x - simd::set1(-1.25f)) + simd::set1(0.14320631f) * (x - simd::set1(-1.25f)) * (x - simd::set1(-1.25f)) + simd::set1(0.28659615f) * (x - simd::set1(-1.25f)) + simd::set1(0.2865048f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-1.0f) <= x, x <= simd::set1(-0.75f)), simd::set1(0.04961379f) * (x - simd::set1(-1.0f)) * (x - simd::set1(-1.0f)) * (x - simd::set1(-1.0f)) + simd::set1(0.18041666f) * (x - simd::set1(-1.0f)) * (x - simd::set1(-1.0f)) + simd::set1(0.36750188f) * (x - simd::set1(-1.0f)) + simd::set1(0.36787945f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-0.75f) <= x, x <= simd::set1(-0.5f)), simd::set1(0.08547847f) * (x - simd::set1(-0.75f)) * (x - simd::set1(-0.75f)) * (x - simd::set1(-0.75f)) + simd::set1(0.2445255f) * (x - simd::set1(-0.75f)) * (x - simd::set1(-0.75f)) + simd::set1(0.47373742f) * (x - simd::set1(-0.75f)) + simd::set1(0.47236654f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-0.5f) <= x, x <= simd::set1(-0.25f)), simd::set1(0.02860214f) * (x - simd::set1(-0.5f)) * (x - simd::set1(-0.5f)) * (x - simd::set1(-0.5f)) + simd::set1(0.2659771f) * (x - simd::set1(-0.5f)) * (x - simd::set1(-0.5f)) + simd::set1(0.60136306f) * (x - simd::set1(-0.5f)) + simd::set1(0.60653067f), value);
    value = simd::ifelse(simd::bit_and(simd::set1(-0.25f) <= x, x <= simd::set1(0.0f)), simd::set1(0.3395703f) * (x - simd::set1(-0.25f)) * (x - simd::set1(-0.25f)) * (x - simd::set1(-0.25f)) + simd::set1(0.52065486f) * (x - simd::set1(-0.25f)) * (x - simd::set1(-0.25f)) + simd::set1(0.7980211f) * (x - simd::set1(-0.25f)) + simd::set1(0.7788008f), value);
    return value;
}
