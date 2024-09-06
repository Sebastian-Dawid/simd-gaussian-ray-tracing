#include "types.h"
#include "approx.h"

namespace vrt
{
    ///
    /// vec4f_t
    ///

    vec4f_t vec4f_t::operator+(const vec4f_t &other) const
    {
        return vec4f_t{ .x = this->x + other.x, .y = this->y + other.y, .z = this->z + other.z, .w = this->w + other.w };
    }
    vec4f_t vec4f_t::operator-(const vec4f_t &other) const
    {
        return vec4f_t{ .x = this->x - other.x, .y = this->y - other.y, .z = this->z - other.z, .w = this->w - other.w };
    }
    vec4f_t vec4f_t::operator*(const f32 &lambda) const
    {
        return vec4f_t{ .x = this->x * lambda, .y = this->y * lambda, .z = this->z * lambda, .w = this->w * lambda };
    }
    vec4f_t vec4f_t::operator/(const vec4f_t &other) const
    {
        return vec4f_t{ .x = this->x / other.x, .y = this->y / other.y, .z = this->z / other.z, .w = this->w / other.w };
    }

    /// Returns the dot product between this vector and `other`.
    f32 vec4f_t::dot(const vec4f_t &other) const
    {
        return this->x * other.x + this->y * other.y + this->z * other.z + this->w * other.w;
    }

    /// Returns the element-wise maximum between this vector and `other`.
    vec4f_t vec4f_t::max(const vec4f_t &other) const
    {
        return vec4f_t{ .x = std::max(this->x, other.x), .y = std::max(this->y, other.y), .z = std::max(this->z, other.z), .w = std::max(this->w, other.w) };
    }

    /// Returns the squared norm of this vector.
    f32 vec4f_t::sqnorm() const
    {
        return this->dot(*this);
    }

    /// Normalizes the vector to unit length.
    void vec4f_t::normalize()
    {
        f32 norm = std::sqrt(this->dot(*this));
        this->x /= norm;
        this->y /= norm;
        this->z /= norm;
        this->w /= norm;
    }
    glm::vec4 vec4f_t::to_glm() const
    {
        return glm::vec4(this->x, this->y, this->z, this->w);
    }
    vec4f_t vec4f_t::from_glm(glm::vec4 other)
    {
        return vec4f_t{other.x, other.y, other.z, other.w};
    }

    ///
    /// simd_vec4f_t
    ///

    simd_vec4f_t simd_vec4f_t::operator+(const simd_vec4f_t &other) const
    {
        return simd_vec4f_t{ this->x + other.x, this->y + other.y, this->z + other.z, this->w + other.w };
    }
    simd_vec4f_t simd_vec4f_t::operator-(const simd_vec4f_t &other) const
    {
        return simd_vec4f_t{ this->x - other.x, this->y - other.y, this->z - other.z, this->w - other.w };
    }
    simd_vec4f_t simd_vec4f_t::operator*(const simd::Vec<simd::Float> &lambda) const
    {
        return simd_vec4f_t{ this->x * lambda, this->y * lambda, this->z * lambda, this->w * lambda };
    }
    simd_vec4f_t simd_vec4f_t::operator/(const simd_vec4f_t &other) const
    {
        return simd_vec4f_t{ this->x / other.x, this->y / other.y, this->z / other.z, this->w / other.w };
    }

    /// Returns the element-wise dot product of this set of vectors with the vectors in `other` in a `simd::Vec<simd::Float>`.
    simd::Vec<simd::Float> simd_vec4f_t::dot(const simd_vec4f_t &other) const
    {
        return this->x * other.x + this->y * other.y + this->z * other.z + this->w * other.w;
    }

    /// Returns the element-wise maxima between this set of vectors and the vectors given in `other`.
    simd_vec4f_t simd_vec4f_t::max(const simd_vec4f_t &other) const
    {
        return simd_vec4f_t{ simd::max(this->x, other.x), simd::max(this->y, other.y), simd::max(this->z, other.z), simd::max(this->w, other.w) };
    }

    /// Returns the squared norms of this set of vectors in a `simd::Vec<simd::Float>`.
    simd::Vec<simd::Float> simd_vec4f_t::sqnorm() const
    {
        return this->dot(*this);
    }

    vec4f_t simd_vec4f_t::hadds() const
    {
        return vec4f_t{ .x = simd::hadds(this->x), .y = simd::hadds(this->y), .z = simd::hadds(this->z), .w = simd::hadds(this->w) };
    }

    /// Normalizes all vectors in this set to unit length.
    void simd_vec4f_t::normalize()
    {
        simd::Vec<simd::Float> norm = simd::sqrt(this->dot(*this));
        this->x /= norm;
        this->y /= norm;
        this->z /= norm;
        this->w /= norm;
    }

    /// Broadcasts a single `vec4f_t` to a set of `SIMD_FLOATS` vectors.
    simd_vec4f_t simd_vec4f_t::from_vec4f_t(const vec4f_t &other)
    {
        return simd_vec4f_t{
            .x = simd::set1(other.x),
                .y = simd::set1(other.y),
                .z = simd::set1(other.z),
                .w = simd::set1(other.w)
        };
    }

    ///
    /// gaussian_t
    ///

    /// Returns the density of the gaussian at point `x`.
    f32 gaussian_t::pdf(vec4f_t x) const
    {
        return this->magnitude * approx::fast_exp(-((x - this->mu).dot(x - this->mu))/(2 * this->sigma * this->sigma));
    }

    ///
    /// gaussian_vec_t
    ///

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
    /// `SIMD_BYTES` aligned memory is allocated for the number of gaussians included in `gaussians`.
    gaussian_vec_t *gaussian_vec_t::from_gaussians(const std::vector<gaussian_t> &gaussians)
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

    gaussian_vec_t::gaussian_vec_t(gaussian_vec_t &other)
    {
        this->size = other.size;
        this->mu.x = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        memcpy(this->mu.x, other.mu.x, this->size * sizeof(f32));
        this->mu.y = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        memcpy(this->mu.y, other.mu.y, this->size * sizeof(f32));
        this->mu.z = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        memcpy(this->mu.z, other.mu.z, this->size * sizeof(f32));
        this->albedo.r = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        memcpy(this->albedo.r, other.albedo.r, this->size * sizeof(f32));
        this->albedo.g = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        memcpy(this->albedo.g, other.albedo.g, this->size * sizeof(f32));
        this->albedo.b = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        memcpy(this->albedo.b, other.albedo.b, this->size * sizeof(f32));
        this->sigma = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        memcpy(this->sigma, other.sigma, this->size * sizeof(f32));
        this->magnitude = (f32*)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * size);
        memcpy(this->magnitude, other.magnitude, this->size * sizeof(f32));
    }

    /// Frees all allocated memory.
    gaussian_vec_t::~gaussian_vec_t()
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

    ///
    /// simd_gaussian_t
    ///

    /// Returns the vector of the densities at the points in `x` of the corresponding gaussians.
    simd::Vec<simd::Float> simd_gaussian_t::pdf(const simd_vec4f_t x) const
    {
        return this->magnitude * approx::simd_fast_exp(-((x - this->mu).dot(x - this->mu))/(simd::set1(2.f) * this->sigma * this->sigma));
    }

    /// Broadcasts a single `gaussian_t` to a set of `SIMD_FLOATS` gaussians.
    simd_gaussian_t simd_gaussian_t::from_gaussian_t(const gaussian_t &other)
    {
        simd_gaussian_t g;
        g.albedo = simd_vec4f_t::from_vec4f_t(other.albedo);
        g.mu = simd_vec4f_t::from_vec4f_t(other.mu);
        g.sigma = simd::set1(other.sigma);
        g.magnitude = simd::set1(other.magnitude);
        return g;
    }
};
