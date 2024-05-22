#pragma once

#include <vector>
#include <cmath>
#include "types.h"
#include <include/tsimd_sh.H>

float transmittance(const vec4f_t o, const vec4f_t n, const float s, const std::vector<gaussian_t> gaussians);
float transmittance_step(const vec4f_t o, const vec4f_t n, const float s, const float delta, const std::vector<gaussian_t> gaussians);
float density(const vec4f_t pt, const std::vector<gaussian_t> gaussians);
vec4f_t l_hat(const vec4f_t o, const vec4f_t n, const std::vector<gaussian_t> gaussians);
