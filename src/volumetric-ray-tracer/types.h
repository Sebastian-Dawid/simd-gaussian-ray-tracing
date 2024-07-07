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

    /// Returns the dot product between this vector and `other`.
    f32 dot(const vec4f_t &other) const
    {
        return this->x * other.x + this->y * other.y + this->z * other.z + this->w * other.w;
    }

    /// Returns the element-wise maximum between this vector and `other`.
    vec4f_t max(const vec4f_t &other) const
    {
        return vec4f_t{ .x = std::max(this->x, other.x), .y = std::max(this->y, other.y), .z = std::max(this->z, other.z), .w = std::max(this->w, other.w) };
    }

    /// Returns the squared norm of this vector.
    f32 sqnorm() const
    {
        return this->dot(*this);
    }

    /// Normalizes the vector to unit length.
    void normalize()
    {
        f32 norm = std::sqrt(this->dot(*this));
        this->x /= norm;
        this->y /= norm;
        this->z /= norm;
        this->w /= norm;
    }

    /// TODO: It might be a better idea to refactor everything to use glm vectors.
    /// Converts this `vec4f_t` instance to glm::vec4.
    glm::vec4 to_glm() const
    {
        return glm::vec4(this->x, this->y, this->z, this->w);
    }
#ifdef INCLUDE_IMGUI
    /// Creates controls for this `vec4f_t` instance. Uniqueness is ensured by using the address of the instance
    /// as its ID.
    void imgui_controls(std::string label = "x, y, z, w", f32 min = -10.f, f32 max = 10.f, bool color = false)
    {
        ImGui::PushID(this);
        if (color)
            ImGui::ColorEdit4(label.c_str(), &this->x);
        else
            ImGui::SliderFloat4(label.c_str(), &this->x, min, max);
        ImGui::PopID();
    }

    /// Creates an ImGui window containing the controls for this `vec4f_t` instance
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

    /// Returns the element-wise dot product of this set of vectors with the vectors in `other` in a `simd::Vec<simd::Float>`.
    simd::Vec<simd::Float> dot(const simd_vec4f_t &other) const
    {
        return this->x * other.x + this->y * other.y + this->z * other.z + this->w * other.w;
    }

    /// Returns the element-wise maxima between this set of vectors and the vectors given in `other`.
    simd_vec4f_t max(const simd_vec4f_t &other) const
    {
        return simd_vec4f_t{ simd::max(this->x, other.x), simd::max(this->y, other.y), simd::max(this->z, other.z), simd::max(this->w, other.w) };
    }

    /// Returns the squared norms of this set of vectors in a `simd::Vec<simd::Float>`.
    simd::Vec<simd::Float> sqnorm() const
    {
        return this->dot(*this);
    }

    /// Normalizes all vectors in this set to unit length.
    void normalize()
    {
        simd::Vec<simd::Float> norm = simd::sqrt(this->dot(*this));
        this->x /= norm;
        this->y /= norm;
        this->z /= norm;
        this->w /= norm;
    }

    /// Broadcasts a single `vec4f_t` to a set of `SIMD_FLOATS` vectors.
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
    
    /// Returns the density of the gaussian at point `x`.
    f32 pdf(vec4f_t x) const
    {
        return this->magnitude * approx::fast_exp(-((x - this->mu).dot(x - this->mu))/(2 * this->sigma * this->sigma));
    }

    /// Returns the density of the gaussian at point `x`. Templated to allow for substitution
    /// of the used approximation of the exponential function.
    template<typename Fn>
    f32 pdf(vec4f_t x, Fn &&_exp = std::expf) const
    {
        return this->magnitude * _exp(-((x - this->mu).dot(x - this->mu))/(2 * this->sigma * this->sigma));
    }
#ifdef INCLUDE_IMGUI
    /// Creates controls for this `gaussian_t` instance. Uniqueness is ensured by using the address of the instance
    /// as its ID.
    void imgui_controls()
    {
        ImGui::PushID(this);
        this->albedo.imgui_controls("albedo", 0.f, 0.f, true);
        this->mu.imgui_controls("mu");
        ImGui::SliderFloat("sigma", &this->sigma, .1f, 1.f);
        ImGui::SliderFloat("magnitude", &this->magnitude, 0.f, 10.f);
        ImGui::PopID();
    }
    /// Creates an ImGui window containing the controls for this `gaussian_t` instance.
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

    /// Loads gaussians from a given `std::vector<gaussian_t>`.
    /// Only loads `this->size` gaussians.
    /// Does not perform reallocations.
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

    /// Creates a new `gaussian_vec_t` from a given `std::vector<gaussian_t>`.
    /// `SIMD_BYTES` aligned memory is allocated for the number of gaussians included in `gaussians`.
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

    /// Frees all allocated memory.
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
    const u64 w, h;

    /// BUG: number of horizontal tiles and vertical tiles should probably be given explicitly
    tiles_t(const std::vector<gaussians_t> &gaussians, const f32 tw, const f32 th) : gaussians(gaussians), tw(tw), th(th), w(std::ceil(2.f/tw)), h(std::ceil(2.f/th)) {}

    /// Destroy all `gaussian_vec_t`s since they will not go out of scope naturally.
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
    
    /// Returns the vector of the densities at the points in `x` of the corresponding gaussians.
    simd::Vec<simd::Float> pdf(const simd_vec4f_t x) const
    {
        return this->magnitude * approx::simd_fast_exp(-((x - this->mu).dot(x - this->mu))/(simd::set1(2.f) * this->sigma * this->sigma));
    }

    /// Returns the vector of the densities at the points in `x` of the corresponding gaussians. Templated to allow for
    /// substitution of the used exponential function.
    template <typename Fn>
    simd::Vec<simd::Float> pdf(const simd_vec4f_t x, const Fn &&_exp) const
    {
        return this->magnitude * _exp(-((x - this->mu).dot(x - this->mu))/(simd::set1(2.f) * this->sigma * this->sigma));
    }

    /// Broadcasts a single `gaussian_t` to a set of `SIMD_FLOATS` gaussians.
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
