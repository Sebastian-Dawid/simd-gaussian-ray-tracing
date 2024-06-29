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
                const glm::vec2 mu(proj.x/proj.z, proj.y/proj.z);
                const f32 sigma = g.sigma / proj.z;
                const glm::vec2 p = glm::abs(glm::vec2(x, y) - mu);
                if ((p.x <= tw/2 + 2 * sigma && p.y <= th/2 + 2 * sigma))
                {
                    gs.gaussians.push_back(g);
                }
            }
            gs.gaussians_broadcast = gaussian_vec_t::from_gaussians(gs.gaussians);
        }
    }

    return tiles_t(tiles, tw, th);
}

vec4f_t l_hat(const vec4f_t o, const vec4f_t n, const gaussians_t &gaussians)
{
    vec4f_t L_hat{ .x = 0.f, .y = 0.f, .z = 0.f };
    for (const gaussian_t &G_q : gaussians.gaussians)
    {
        const f32 lambda_q = G_q.sigma;
        f32 inner = 0.f;
        for (i8 k = -4; k <= 0; ++k)
        {
            const f32 s = (G_q.mu - o).dot(n) + k * lambda_q;
            f32 T = transmittance(o, n, s, gaussians, approx::fast_exp, approx::abramowitz_stegun_erf);
            inner += G_q.pdf(o + (n * s)) * T * lambda_q;
        }
        L_hat = L_hat + (G_q.albedo * inner);
    }
    return L_hat;
}


bool render_image(const u32 width, const u32 height, u32 *image, const u32 *bg_image, const f32 *xs, const f32 *ys, const gaussians_t &gaussians, const bool &running)
{
    static bool is_wayland_display = getenv("WAYLAND_DISPLAY") != NULL;
    static const vec4f_t origin = { 0.f, 0.f, 0.f };

    for (u64 i = 0; i < width * height; ++i)
    {
        vec4f_t dir = vec4f_t{ xs[i], ys[i], 1.f } - origin;
        dir.normalize();
        const vec4f_t color = l_hat(origin, dir, gaussians);
        const u32 bg = bg_image[i];
        const u32 A = 0xFF000000; // final alpha channel is always 1
        const u32 R = (u32)((1 - color.w) * ((0x00FF0000 & bg) >> 16) + std::min(color.x, 1.0f) * 255);
        const u32 G = (u32)((1 - color.w) * ((0x0000FF00 & bg) >> 8) + std::min(color.y, 1.0f) * 255);
        const u32 B = (u32)((1 - color.w) * ((0x000000FF & bg)) + std::min(color.z, 1.0f) * 255);
        if (is_wayland_display)
            image[i] = A | R | G << 8 | B << 16;
        else
            image[i] = A | R << 16 | G << 8 | B;
        if (!running) return true;
    }
    return false;
}

bool render_image(const u32 width, const u32 height, u32 *image, const u32 *bg_image, const f32 *xs, const f32 *ys, const tiles_t &tiles, const bool &running)
{
    static bool is_wayland_display = getenv("WAYLAND_DISPLAY") != NULL;
    static const vec4f_t origin = { 0.f, 0.f, 0.f };
    u64 tidx = 0;

    for (u64 i = 0; i < width * height; ++i)
    {
        tidx = u64((xs[i] + 1.f)/tiles.tw) + u64((ys[i] + 1.f)/tiles.th) * tiles.w;
        vec4f_t dir = vec4f_t{ xs[i], ys[i], 1.f } - origin;
        dir.normalize();
        const vec4f_t color = l_hat(origin, dir, tiles.gaussians[tidx]);
        const u32 bg = bg_image[i];
        const u32 A = 0xFF000000; // final alpha channel is always 1
        // NOTE: clamp color values to [0, 1]
        const u32 R = (u32)((1 - color.w) * ((0x00FF0000 & bg) >> 16) + std::min(color.x, 1.0f) * 255);
        const u32 G = (u32)((1 - color.w) * ((0x0000FF00 & bg) >> 8) + std::min(color.y, 1.0f) * 255);
        const u32 B = (u32)((1 - color.w) * ((0x000000FF & bg)) + std::min(color.z, 1.0f) * 255);
        if (is_wayland_display)
            image[i] = A | R | G << 8 | B << 16;
        else
            image[i] = A | R << 16 | G << 8 | B;
        if (!running) return true;
    }
    return false;
}
