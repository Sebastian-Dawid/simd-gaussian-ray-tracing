#pragma once

#include <include/tsimd_sh.H>
#include <include/definitions.h>
#include <cmath>

#include <fmt/core.h>
#include <fmt/format.h>
#include <vector>
#ifdef INCLUDE_IMGUI
#include <imgui.h>
#endif
#include <string>

#include "approx.h"

struct vec4f_t
{
    float x, y, z, w = 0.f;
    vec4f_t operator+(const vec4f_t &other) const
    {
        return vec4f_t{ .x = this->x + other.x, .y = this->y + other.y, .z = this->z + other.z, .w = this->w + other.w };
    }
    vec4f_t operator-(const vec4f_t &other) const
    {
        return vec4f_t{ .x = this->x - other.x, .y = this->y - other.y, .z = this->z - other.z, .w = this->w - other.w };
    }
    vec4f_t operator*(const float &lambda) const
    {
        return vec4f_t{ .x = this->x * lambda, .y = this->y * lambda, .z = this->z * lambda, .w = this->w * lambda };
    }
    vec4f_t operator/(const vec4f_t &other) const
    {
        return vec4f_t{ .x = this->x / other.x, .y = this->y / other.y, .z = this->z / other.z, .w = this->w / other.w };
    }
    float dot(const vec4f_t &other) const
    {
        return this->x * other.x + this->y * other.y + this->z * other.z + this->w * other.w;
    }
    vec4f_t max(const vec4f_t &other) const
    {
        return vec4f_t{ .x = std::max(this->x, other.x), .y = std::max(this->y, other.y), .z = std::max(this->z, other.z), .w = std::max(this->w, other.w) };
    }
    void normalize()
    {
        float norm = std::sqrt(this->dot(*this));
        this->x /= norm;
        this->y /= norm;
        this->z /= norm;
        this->w /= norm;
    }
#ifdef INCLUDE_IMGUI
    void imgui_controls(std::string label = "x, y, z, w", float min = -10.f, float max = 10.f, bool color = false)
    {
        ImGui::PushID(this);
        if (color)
            ImGui::ColorEdit4(label.c_str(), &this->x);
        else
            ImGui::SliderFloat4(label.c_str(), &this->x, min, max);
        ImGui::PopID();
    }
    void imgui_window(std::string name = "Vec4f")
    {
        ImGui::Begin(name.c_str());
        this->imgui_controls();
        ImGui::End();
    }
#endif
};

struct simd_vec4f_t
{
    simd::Vec<simd::Float> x, y, z, w = simd::set1(0.f);
    simd_vec4f_t operator+(const simd_vec4f_t &other) const
    {
        return simd_vec4f_t{ this->x + other.x, this->y + other.y, this->z + other.z, this->w + other.w };
    }
    simd_vec4f_t operator-(const simd_vec4f_t &other) const
    {
        return simd_vec4f_t{ this->x - other.x, this->y - other.y, this->z - other.z, this->w - other.w };
    }
    simd_vec4f_t operator*(const simd::Vec<simd::Float> &lambda) const
    {
        return simd_vec4f_t{ this->x * lambda, this->y * lambda, this->z * lambda, this->w * lambda };
    }
    simd_vec4f_t operator/(const simd_vec4f_t &other) const
    {
        return simd_vec4f_t{ this->x / other.x, this->y / other.y, this->z / other.z, this->w / other.w };
    }
    simd::Vec<simd::Float> dot(const simd_vec4f_t &other) const
    {
        return this->x * other.x + this->y * other.y + this->z * other.z + this->w * other.w;
    }
    simd_vec4f_t max(const simd_vec4f_t &other) const
    {
        return simd_vec4f_t{ simd::max(this->x, other.x), simd::max(this->y, other.y), simd::max(this->z, other.z), simd::max(this->w, other.w) };
    }
    void normalize()
    {
        simd::Vec<simd::Float> norm = simd::sqrt(this->dot(*this));
        this->x /= norm;
        this->y /= norm;
        this->z /= norm;
        this->w /= norm;
    }

    static simd_vec4f_t from_vec4f_t(const vec4f_t &other)
    {
        return simd_vec4f_t{
            .x = simd::set1(other.x),
            .y = simd::set1(other.y),
            .z = simd::set1(other.z),
            .w = simd::set1(other.w)
        };
    }
};

struct gaussian_t
{
    vec4f_t albedo;
    vec4f_t mu;
    float sigma;
    float magnitude;
    float pdf(vec4f_t x) const
    {
        return this->magnitude * _exp(-((x - this->mu).dot(x - this->mu))/(2 * this->sigma * this->sigma));
    }
#ifdef INCLUDE_IMGUI
    void imgui_controls()
    {
        ImGui::PushID(this);
        this->albedo.imgui_controls("albedo", 0.f, 0.f, true);
        this->mu.imgui_controls("mu");
        ImGui::SliderFloat("sigma", &this->sigma, 0.f, 1.f);
        ImGui::SliderFloat("magnitude", &this->magnitude, 0.f, 10.f);
        ImGui::PopID();
    }
    void imgui_window(std::string name = "Gaussian")
    {
        ImGui::Begin(name.c_str());
        this->imgui_controls();
        ImGui::End();
    }
#endif
};

// for easier loading into simd gaussians
struct gaussian_vec_t
{
    struct
    {
        float *r;
        float *g;
        float *b;
    } albedo;
    struct
    {
        float *x;
        float *y;
        float *z;
    } mu;
    float *sigma;
    float *magnitude;
    u64 size;

    void load_gaussians(const std::vector<gaussian_t> &gaussians)
    {
        for (u64 i = 0; i < this->size; ++i)
        {
            if (i >= gaussians.size())
            {
                this->mu.x[i]      = 0.f;
                this->mu.y[i]      = 0.f;
                this->mu.z[i]      = 0.f;
                this->albedo.r[i]  = 0.f;
                this->albedo.g[i]  = 0.f;
                this->albedo.b[i]  = 0.f;
                this->sigma[i]     = 1.f;
                this->magnitude[i] = 0.f;
                continue;
            }
            this->mu.x[i]      = gaussians[i].mu.x;
            this->mu.y[i]      = gaussians[i].mu.y;
            this->mu.z[i]      = gaussians[i].mu.z;
            this->albedo.r[i]  = gaussians[i].albedo.x;
            this->albedo.g[i]  = gaussians[i].albedo.y;
            this->albedo.b[i]  = gaussians[i].albedo.z;
            this->sigma[i]     = gaussians[i].sigma;
            this->magnitude[i] = gaussians[i].magnitude;
        }
    }

    static gaussian_vec_t from_gaussians(const std::vector<gaussian_t> &gaussians)
    {
        gaussian_vec_t vec;
        u64 size = ((gaussians.size() / SIMD_FLOATS) + 1) * SIMD_FLOATS;

        vec.mu.x = (float*)std::aligned_alloc(SIMD_BYTES, sizeof(float) * size);
        vec.mu.y = (float*)std::aligned_alloc(SIMD_BYTES, sizeof(float) * size);
        vec.mu.z = (float*)std::aligned_alloc(SIMD_BYTES, sizeof(float) * size);
        vec.albedo.r = (float*)std::aligned_alloc(SIMD_BYTES, sizeof(float) * size);
        vec.albedo.g = (float*)std::aligned_alloc(SIMD_BYTES, sizeof(float) * size);
        vec.albedo.b = (float*)std::aligned_alloc(SIMD_BYTES, sizeof(float) * size);
        vec.sigma = (float*)std::aligned_alloc(SIMD_BYTES, sizeof(float) * size);
        vec.magnitude = (float*)std::aligned_alloc(SIMD_BYTES, sizeof(float) * size);

        for (u64 i = 0; i < size; ++i)
        {
            if (i >= gaussians.size())
            {
                vec.mu.x[i]      = 0.f;
                vec.mu.y[i]      = 0.f;
                vec.mu.z[i]      = 0.f;
                vec.albedo.r[i]  = 0.f;
                vec.albedo.g[i]  = 0.f;
                vec.albedo.b[i]  = 0.f;
                vec.sigma[i]     = 1.f;
                vec.magnitude[i] = 0.f;
                continue;
            }
            vec.mu.x[i]      = gaussians[i].mu.x;
            vec.mu.y[i]      = gaussians[i].mu.y;
            vec.mu.z[i]      = gaussians[i].mu.z;
            vec.albedo.r[i]  = gaussians[i].albedo.x;
            vec.albedo.g[i]  = gaussians[i].albedo.y;
            vec.albedo.b[i]  = gaussians[i].albedo.z;
            vec.sigma[i]     = gaussians[i].sigma;
            vec.magnitude[i] = gaussians[i].magnitude;
        }
        vec.size = size;
        return vec;
    }

    ~gaussian_vec_t()
    {
        free(this->mu.x);
        free(this->mu.y);
        free(this->mu.z);
        free(this->albedo.r);
        free(this->albedo.g);
        free(this->albedo.b);
        free(this->sigma);
        free(this->magnitude);
    }
};

struct gaussians_t
{
    std::vector<gaussian_t> gaussians;
    gaussian_vec_t gaussians_broadcast;
};

struct simd_gaussian_t
{
    simd_vec4f_t albedo;
    simd_vec4f_t mu;
    simd::Vec<simd::Float> sigma;
    simd::Vec<simd::Float> magnitude;
    simd::Vec<simd::Float> pdf(const simd_vec4f_t x) const
    {
        return this->magnitude * _simd_exp(-((x - this->mu).dot(x - this->mu))/(simd::set1(2.f) * this->sigma * this->sigma));
    }
    static simd_gaussian_t from_gaussian_t(const gaussian_t &other)
    {
        simd_gaussian_t g;
        g.albedo = simd_vec4f_t::from_vec4f_t(other.albedo);
        g.mu = simd_vec4f_t::from_vec4f_t(other.mu);
        g.sigma = simd::set1(other.sigma);
        g.magnitude = simd::set1(other.magnitude);
        return g;
    }
};
