#pragma once

#include "include/definitions.h"
#include <include/tsimd_sh.H>

#define VCL_NAMESPACE vcl
#include <include/vectorclass/vectorclass.h>
#include <include/vectorclass/vectormath_exp.h>

namespace approx {
    /// Approximation of the error function using cubic spline interpolation.
    f32 spline_erf(f32 x);
    /// Version of `spline_erf` operating on `simd::Vec<simd::Float>`s instead of single floats.
    simd::Vec<simd::Float> simd_spline_erf(simd::Vec<simd::Float> x);

    /// Approximation of the error function using cubic spline interpolation.
    /// Exploiting the point symmetry of the error function leading to less
    /// cases that need to be checked.
    f32 spline_erf_mirror(f32 x);
    /// Version of `spline_erf_mirror` operating on `simd::Vec<simd::Float>`s instead of single floats.
    simd::Vec<simd::Float> simd_spline_erf_mirror(simd::Vec<simd::Float> x);

    /// Taylor approximation of the error function using the first four terms.
    f32 taylor_erf(f32 x);

    /// Implementation of the error function using the approximation from
    /// Abramowitz and Steguns "Handbook of Mathematical Functions" Section 7.1.27.
    /// https://personal.math.ubc.ca/~cbm/aands/page_299.htm
    f32 abramowitz_stegun_erf(f32 x);
    /// Version of `abramowitz_stegun_erf` operating on `simd::Vec<simd::Float>`s instead of single floats.
    simd::Vec<simd::Float> simd_abramowitz_stegun_erf(simd::Vec<simd::Float> x);

    /// SIMD implementation of the error function from intels SVML library.
    /// Requires linking SVML which is distributed through intels icx compiler.
    extern "C" __m256 __svml_erff8(__m256);
    extern "C" __m512 __svml_erff16(__m512);

    /// Fast implementation of the exponential function based on:
    /// https://gist.github.com/jrade/293a73f89dfef51da6522428c857802d.
    /// PAPER: https://nic.schraudolph.org/pubs/Schraudolph99.pdf
    f32 fast_exp(f32 x);
    /// Version of `fast_exp` operating on `simd::Vec<simd::Float>`s instead of single floats.
    simd::Vec<simd::Float> simd_fast_exp(simd::Vec<simd::Float> x);

    /// Approximation of the exponential function using cubic spline interpolation
    f32 spline_exp(f32 x);
    /// Version of `spline_erf` operating on `simd::Vec<simd::Float>`s instead of single floats
    simd::Vec<simd::Float> simd_spline_exp(simd::Vec<simd::Float> x);

    /// SIMD implementation of the exponential function from intels SVML library.
    /// Requires linking SVML which is distributed through intels icx compiler.
    extern "C" __m256 __svml_expf8(__m256);
    extern "C" __m512 __svml_expf16(__m512);
}

namespace simd {
    inline simd::Vec<simd::Float> erf(simd::Vec<simd::Float> x)
    {
#ifndef WITH_SVML
        return approx::simd_abramowitz_stegun_erf(x);
#else
#ifndef __AVX512F__
        return approx::__svml_erff8(x);
#else
        return approx::__svml_erff16(x);
#endif
#endif
    }
    inline simd::Vec<simd::Float> exp(simd::Vec<simd::Float> x)
    {
#ifndef __AVX512F__
#ifndef WITH_SVML
        return (__m256)vcl::exp(static_cast<__m256>(x));
#else
        return approx::__svml_expf8(x);
#endif
#else
#ifndef WITH_SVML
        return (__m512)vcl::exp(static_cast<__m512>(x));
#else
        return approx::__svml_expf16(x);
#endif
#endif
    }
}

// TODO: replace occurrences with templates. Test impact!
// NOTE: Won't this require everything that that uses these to be templated?
//       It would seem so...
inline f32 (*_erf)(f32) = std::erf;
inline f32 (*_exp)(f32) = std::exp;
inline simd::Vec<simd::Float> (*_simd_erf)(simd::Vec<simd::Float>) = approx::simd_abramowitz_stegun_erf;
inline simd::Vec<simd::Float> (*_simd_exp)(simd::Vec<simd::Float>) = approx::simd_fast_exp;
