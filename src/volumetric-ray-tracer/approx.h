#pragma once

#include <include/tsimd_sh.H>

float spline_erf(float x);
simd::Vec<simd::Float> simd_spline_erf(simd::Vec<simd::Float> x);
float spline_erf_mirror(float x);
simd::Vec<simd::Float> simd_spline_erf_mirror(simd::Vec<simd::Float> x);
float taylor_erf(float x);

// TODO: erf approx via newtons mehtod and Mike Giles:
//       http://www.mimirgames.com/articles/programming/approximations-of-the-inverse-error-function/

// refernece: https://nic.schraudolph.org/pubs/Schraudolph99.pdf
// https://gist.github.com/jrade/293a73f89dfef51da6522428c857802d
float fast_exp(float x);
simd::Vec<simd::Float> simd_fast_exp(simd::Vec<simd::Float> x);
float spline_exp(float x);
simd::Vec<simd::Float> simd_spline_exp(simd::Vec<simd::Float> x);
