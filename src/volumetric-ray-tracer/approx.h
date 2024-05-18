#pragma once

#include <include/tsimd_sh.H>

float spline_erf(float x);
simd::Vec<simd::Float> simd_spline_erf(simd::Vec<simd::Float> x);
float spline_erf_mirror(float x);
simd::Vec<simd::Float> simd_spline_erf_mirror(simd::Vec<simd::Float> x);
float taylor_erf(float x);

float spline_exp(float x);
simd::Vec<simd::Float> simd_spline_exp(simd::Vec<simd::Float> x);
