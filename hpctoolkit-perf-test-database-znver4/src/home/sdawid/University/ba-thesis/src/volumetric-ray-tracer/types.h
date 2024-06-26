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

#include <glm/glm.hpp>

struct vec4f_t
{
    f32 x, y, z, w = 0.f;
    vec4f_t operator+(const vec4f_t &other) const
    {
        return vec4f_t{ .x = this->x + other.x, .y = this->y + other.y, .z = this->z + other.z, .w = this->w + other.w };
    }
    vec4f_t operator-(const vec4f_t &other) const
    {
        return vec4f_t{ .x = this->x - other.x, .y = this->y - other.y, .z = this->z - other.z, .w = this->w - other.w };
    }
    vec4f_t operator*(const f32 &lambda) const
    {
        return vec4f_t{ .x = this->x * lambda, .y = this->y * lambda, .z = this->z * lambda, .w = this->w * lambda };
    }
    vec4f_t operator/(const vec4f_t &other) const
    {
        return vec4f_t{ .x = this->x / other.x, .y = this->y / other.y, .z = this->z / other.z, .w = this->w / other.w };
    }
    f32 dot(const vec4f_t &other) const
    {
        return this->x * other.x + this->y * other.y + this->z * other.z + this->w * other.w;
    }
    vec4f_t max(const vec4f_t &other) const
    {
        return vec4f_t{ .x = std::max(this->x, other.x), .y = std::max(this->y, other.y), .z = std::max(this->z, other.z), .w = std::max(this->w, other.w) };
    }
    f32 sqnorm() const
    {
        return this->dot(*this);
    }
    void normalize()
    {
        f32 norm = std::sqrt(this->dot(*this));
        this->x /= norm;
        this->y /= norm;
        this->z /= norm;
        this->w /= norm;
    }
    // TODO: It might be a better idea to refactor everything to use glm vectors.
    glm::vec4 to_glm() const
    {
        return glm::vec4(this->x, this->y, this->z, this->w);
    }
#ifdef INCLUDE_IMGUI
    void imgui_controls(std::string label = "x, y, z, w", f32 min = -10.f, f32 max = 10.f, bool color = false)
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
    simd::Vec<simd::Float> sqnorm() const {
        return this->dot(*this);
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
    f32 sigma;
    f32 magnitude;
    f32 pdf(vec4f_t x) const
    {
        return this->magnitude * _exp(-((x - this->mu).dot(x - this->mu))/(2 * this->sigma * this->sigma));
    }
#ifdef INCLUDE_IMGUI
    void imgui_controls()
    {
        ImGui::PushID(this);
        this->albedo.imgui_controls("albedo", 0.f, 0.f, true);
        this->mu.imgui_controls("mu");
        ImGui::SliderFloat("sigma", &this->sigma, .1f, 1.f);
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
        f32 *r = nullptr;
        f32 *g = nullptr;
        f32 *b = nullptr;
    } albedo;
    struct
    {
        f32 *x = nullptr;
        f32 *y = nullptr;
        f32 *z = nullptr;
    } mu;
    f32 *sigma = nullptr;
    f32 *magnitude = nullptr;
    u64 size = 0;

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

    static gaussian_vec_t *from_gaussians(const std::vector<gaussian_t> &gaussians)
    {
        gaussian_vec_t *vec = new gaussian_vec_t();
        u64 size = ((gaussians.size() / SIMD_FLOATS) + 1) * SIMD_FLOATS;

        vec->mu.x = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        vec->mu.y = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        vec->mu.z = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        vec->albedo.r = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        vec->albedo.g = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        vec->albedo.b = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        vec->sigma = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        vec->magnitude = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);

        for (u64 i = 0; i < size; ++i)
        {
            if (i >= gaussians.size())
            {
                vec->mu.x[i]      = 0.f;
                vec->mu.y[i]      = 0.f;
                vec->mu.z[i]      = 0.f;
                vec->albedo.r[i]  = 0.f;
                vec->albedo.g[i]  = 0.f;
                vec->albedo.b[i]  = 0.f;
                vec->sigma[i]     = 1.f;
                vec->magnitude[i] = 0.f;
                continue;
            }
            vec->mu.x[i]      = gaussians[i].mu.x;
            vec->mu.y[i]      = gaussians[i].mu.y;
            vec->mu.z[i]      = gaussians[i].mu.z;
            vec->albedo.r[i]  = gaussians[i].albedo.x;
            vec->albedo.g[i]  = gaussians[i].albedo.y;
            vec->albedo.b[i]  = gaussians[i].albedo.z;
            vec->sigma[i]     = gaussians[i].sigma;
            vec->magnitude[i] = gaussians[i].magnitude;
        }
        vec->size = size;
        return vec;
    }

    ~gaussian_vec_t()
    {
        if (this->mu.x) simd_aligned_free(this->mu.x);
        if (this->mu.y) simd_aligned_free(this->mu.y);
        if (this->mu.z) simd_aligned_free(this->mu.z);
        if (this->albedo.r) simd_aligned_free(this->albedo.r);
        if (this->albedo.g) simd_aligned_free(this->albedo.g);
        if (this->albedo.b) simd_aligned_free(this->albedo.b);
        if (this->sigma) simd_aligned_free(this->sigma);
        if (this->magnitude) simd_aligned_free(this->magnitude);
    }
};

struct gaussians_t
{
    std::vector<gaussian_t> gaussians;
    gaussian_vec_t *gaussians_broadcast = nullptr;
};

struct tiles_t
{
    const std::vector<gaussians_t> gaussians;
    const f32 tw;
    const f32 th;
    const u64 w;

    tiles_t(const std::vector<gaussians_t> &gaussians, const f32 tw, const f32 th) : gaussians(gaussians), tw(tw), th(th), w(2.f/tw) {}
    ~tiles_t() {
        for (const gaussians_t &g : this->gaussians)
            delete g.gaussians_broadcast;
    }
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
