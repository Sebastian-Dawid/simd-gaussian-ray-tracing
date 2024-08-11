#include "rt.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <include/definitions.h>
#include <glm/ext/matrix_transform.hpp>

f32 transmittance_step(const vec4f_t o, const vec4f_t n, const f32 s, const f32 delta, const std::vector<gaussian_t> gaussians)
{
    f32 T = 0.f;
    for (f32 t = 0; t <= s; t += delta)
    {
        for (const gaussian_t &g : gaussians)
            T += delta * g.pdf(o + n * t);
    }
    return approx::fast_exp(-T);
}

f32 density(const vec4f_t pt, const std::vector<gaussian_t> gaussians)
{
    f32 D = 0.f;
    for (const gaussian_t &g : gaussians)
    {
        D += g.pdf(pt);
    }
    return D;
}

tiles_t tile_gaussians(const f32 tw, const f32 th, const std::vector<gaussian_t> &gaussians, const glm::mat4 &view)
{
    std::vector<gaussians_t> tiles;
    std::vector<glm::vec2> projected_mu;
    std::vector<f32> projected_sigma;
    std::vector<u64> idxs;
    for (u64 i = 0; i < gaussians.size(); ++i)
    {
        const glm::vec4 proj = view * glm::vec4(glm::vec3(gaussians[i].mu.to_glm()), 1.f);
        if (proj.z < 1.f) continue;
        const glm::vec2 mu(proj.x/proj.z, proj.y/proj.z);
        const f32 sigma = gaussians[i].sigma / proj.z;
        if (sigma < 1e-5f) continue;
        projected_mu.push_back(mu);
        projected_sigma.push_back(sigma);
        idxs.push_back(i);
    }

    for (f32 y = -1.f + th/2; y < 1.f; y += th)
    {
        for (f32 x = -1.f + tw/2; x < 1.f; x += tw)
        {
            tiles.push_back(gaussians_t());
            gaussians_t &gs = tiles.back();
            for (u64 i = 0; i < idxs.size(); ++i)
            {
                const glm::vec2 &mu = projected_mu[i];
                const f32 &sigma = projected_sigma[i];
                const glm::vec2 p = glm::abs(glm::vec2(x, y) - mu);
                if ((p.x <= std::abs(x)/2 + tw/2 + 4.f * sigma
                            && p.y <= std::abs(y)/2 + th/2 + 4.f * sigma))
                {
                    gs.gaussians.push_back(gaussians[idxs[i]]);
                }
            }
            gs.gaussians_broadcast = gaussian_vec_t::from_gaussians(gs.gaussians);
        }
    }

    return tiles_t(tiles, tw, th);
}
