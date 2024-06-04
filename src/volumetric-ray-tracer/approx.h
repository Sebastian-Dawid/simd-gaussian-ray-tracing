#pragma once

#include "include/definitions.h"
#include <include/tsimd_sh.H>

f32 spline_erf(f32 x);
simd::Vec<simd::Float> simd_spline_erf(simd::Vec<simd::Float> x);
f32 spline_erf_mirror(f32 x);
simd::Vec<simd::Float> simd_spline_erf_mirror(simd::Vec<simd::Float> x);
f32 taylor_erf(f32 x);

f32 abramowitz_stegun_erf(f32 x);
simd::Vec<simd::Float> simd_abramowitz_stegun_erf(simd::Vec<simd::Float> x);

// refernece: https://nic.schraudolph.org/pubs/Schraudolph99.pdf
// https://gist.github.com/jrade/293a73f89dfef51da6522428c857802d
f32 fast_exp(f32 x);
simd::Vec<simd::Float> simd_fast_exp(simd::Vec<simd::Float> x);
f32 spline_exp(f32 x);
simd::Vec<simd::Float> simd_spline_exp(simd::Vec<simd::Float> x);

inline f32 (*_erf)(f32) = std::erf;
inline f32 (*_exp)(f32) = std::exp;
inline simd::Vec<simd::Float> (*_simd_erf)(simd::Vec<simd::Float>) = simd_abramowitz_stegun_erf;
inline simd::Vec<simd::Float> (*_simd_exp)(simd::Vec<simd::Float>) = simd_fast_exp;
