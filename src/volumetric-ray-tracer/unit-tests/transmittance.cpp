#include <gtest/gtest.h>
#include "../rt.h"

class vrt_test_transmittance : public testing::Test
{
    protected:
        gaussians_t sg, lg;
        const vec4f_t o = {0.f, 0.f, 0.f}, n = {0.f, 0.f, 1.f};
        std::array<f32, 10> inputs_small, inputs_large;
        std::array<f32, 10> expected_small, expected_large;
        virtual void SetUp()
        {
            sg.gaussians = { gaussian_t{ {}, {0.f, 0.f, 1.f}, 0.1f, 3.0f } };
            std::ranges::generate_n(inputs_small.begin(), inputs_small.size(), [n {0.9f}, this] () mutable { f32 old = n; n += 0.2f/inputs_small.size(); return old; });
            for (u64 i = 0; i < inputs_small.size(); ++i) expected_small[i] = transmittance(o, n, inputs_small[i], sg);
            lg.gaussians = { gaussian_t{ {}, {0.f, 0.f, 1.f}, 0.5f, 3.0f } };
            std::ranges::generate_n(inputs_large.begin(), inputs_large.size(), [n {0.5f}, this] () mutable { f32 old = n; n += 1.f/inputs_large.size(); return old; });
            for (u64 i = 0; i < inputs_large.size(); ++i) expected_large[i] = transmittance(o, n, inputs_large[i], lg);
        }
        virtual void TearDown()
        {}
};

TEST_F(vrt_test_transmittance, transmittance_is_acurate_at_a_small_gaussians_center)
{
    for (u64 i = 0; i < this->inputs_small.size(); ++i)
    {
        f32 got = transmittance(o, n, this->inputs_small[i], this->sg, expf, approx::abramowitz_stegun_erf);
        EXPECT_LE(abs(got - this->expected_small[i]), 1e-3);
    }
}

TEST_F(vrt_test_transmittance, transmittance_is_acurate_at_a_large_gaussians_center)
{
    for (u64 i = 0; i < this->inputs_large.size(); ++i)
    {
        f32 got = transmittance(o, n, this->inputs_large[i], this->lg, expf, approx::abramowitz_stegun_erf);
        EXPECT_LE(abs(got - this->expected_large[i]), 1e-3);
    }
}

TEST_F(vrt_test_transmittance, simd_transmittance_is_acurate_at_a_small_gaussians_center)
{
    for (u64 i = 0; i < this->inputs_small.size(); ++i)
    {
        simd::Vec<simd::Float> _got = broadcast_transmittance(simd_vec4f_t::from_vec4f_t(o),
                simd_vec4f_t::from_vec4f_t(n), simd::set1(this->inputs_small[i]), this->sg, simd::exp, simd::erf);
        f32 got[SIMD_FLOATS];
        simd::storeu(got, _got);
        EXPECT_LE(abs(got[0] - this->expected_small[i]), 1e-4);
    }
}

TEST_F(vrt_test_transmittance, simd_transmittance_is_acurate_at_a_large_gaussians_center)
{
    for (u64 i = 0; i < this->inputs_large.size(); ++i)
    {
        simd::Vec<simd::Float> _got = broadcast_transmittance(simd_vec4f_t::from_vec4f_t(o),
                simd_vec4f_t::from_vec4f_t(n), simd::set1(this->inputs_large[i]), this->lg, simd::exp, simd::erf);
        f32 got[SIMD_FLOATS];
        simd::storeu(got, _got);
        EXPECT_LE(abs(got[0] - this->expected_large[i]), 1e-4);
    }
}
