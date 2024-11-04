#include "types.h"

namespace vrt
{
    /// Loads gaussians from a given `std::vector<gaussian_t>`.
    /// Only loads `this->size` gaussians.
    /// Does not perform reallocations.
    void gaussian_vec_t::load_gaussians(const std::vector<gaussian_t> &gaussians)
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
    /// `NATIVE_SIMD_WIDTH` aligned memory is allocated for the number of gaussians included in `gaussians`.
    gaussian_vec_t *gaussian_vec_t::from_gaussians(const std::vector<gaussian_t> &gaussians)
    {
        gaussian_vec_t *vec = new gaussian_vec_t();
        u64 size = ((gaussians.size() / SIMD_FLOATS) + 1) * SIMD_FLOATS;

        vec->mu.x = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        vec->mu.y = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        vec->mu.z = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        vec->albedo.r = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        vec->albedo.g = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        vec->albedo.b = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        vec->sigma = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        vec->magnitude = (f32*)simd::aligned_malloc(sizeof(f32) * size);

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

    gaussian_vec_t::gaussian_vec_t(gaussian_vec_t &other)
    {
        this->size = other.size;
        this->mu.x = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        memcpy(this->mu.x, other.mu.x, this->size * sizeof(f32));
        this->mu.y = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        memcpy(this->mu.y, other.mu.y, this->size * sizeof(f32));
        this->mu.z = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        memcpy(this->mu.z, other.mu.z, this->size * sizeof(f32));
        this->albedo.r = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        memcpy(this->albedo.r, other.albedo.r, this->size * sizeof(f32));
        this->albedo.g = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        memcpy(this->albedo.g, other.albedo.g, this->size * sizeof(f32));
        this->albedo.b = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        memcpy(this->albedo.b, other.albedo.b, this->size * sizeof(f32));
        this->sigma = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        memcpy(this->sigma, other.sigma, this->size * sizeof(f32));
        this->magnitude = (f32*)simd::aligned_malloc(sizeof(f32) * size);
        memcpy(this->magnitude, other.magnitude, this->size * sizeof(f32));
    }

    /// Frees all allocated memory.
    gaussian_vec_t::~gaussian_vec_t()
    {
        if (this->mu.x) simd::aligned_free(this->mu.x);
        if (this->mu.y) simd::aligned_free(this->mu.y);
        if (this->mu.z) simd::aligned_free(this->mu.z);
        if (this->albedo.r) simd::aligned_free(this->albedo.r);
        if (this->albedo.g) simd::aligned_free(this->albedo.g);
        if (this->albedo.b) simd::aligned_free(this->albedo.b);
        if (this->sigma) simd::aligned_free(this->sigma);
        if (this->magnitude) simd::aligned_free(this->magnitude);
    }

    /// Broadcasts a single `gaussian_t` to a set of `SIMD_FLOATS` gaussians.
    simd_gaussian_t simd_gaussian_t::from_gaussian_t(const gaussian_t &other)
    {
        simd_gaussian_t g;
        g.albedo = simd_vec4f_t::from_vec4f_t(other.albedo);
        g.mu = simd_vec4f_t::from_vec4f_t(other.mu);
        g.sigma = simd::set1<simd::Float>(other.sigma);
        g.magnitude = simd::set1<simd::Float>(other.magnitude);
        return g;
    }
};
