#include "rt.h"
#include <include/definitions.h>

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

    for (f32 y = -1.f + th/2; y < 1.f; y += th)
    {
        for (f32 x = -1.f + tw/2; x < 1.f; x += tw)
        {
            tiles.push_back(gaussians_t());
            gaussians_t &gs = tiles.back();
            for (const gaussian_t &g : gaussians)
            {
                const glm::vec4 proj = view * g.mu.to_glm();
                const glm::vec2 mu(proj.x / proj.z, proj.y/proj.z);
                const f32 sigma = g.sigma / proj.z;
                const glm::vec2 p = glm::abs(glm::vec2(x, y) - mu);
                if ((p.x <= tw/2 + 3.f * sigma && p.y <= th/2 + 3.f * sigma))
                {
                    gs.gaussians.push_back(g);
                }
            }
            gs.gaussians_broadcast = gaussian_vec_t::from_gaussians(gs.gaussians);
        }
    }

    return tiles_t(tiles, tw, th);
}
