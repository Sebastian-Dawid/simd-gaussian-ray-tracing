#include "rt.h"
#include <include/definitions.h>

constexpr f32 SQRT_2_PI = 0.7978845608028654f;
constexpr f32 SQRT_2 = 1.4142135623730951f;

f32 transmittance(const vec4f_t o, const vec4f_t n, const f32 s, const gaussians_t &gaussians)
{
    f32 T = 0.f;
    for (const gaussian_t &g_q : gaussians.gaussians)
    {
        const vec4f_t center_to_origin = g_q.mu - o;
        const f32 mu_bar = (center_to_origin).dot(n);
        const f32 c_bar = g_q.magnitude * EXP( -((center_to_origin).sqnorm() - std::pow(mu_bar, 2.f))/(2*std::pow(g_q.sigma, 2.f)) );
        const f32 sqrt_2_sig = SQRT_2 * g_q.sigma;
        T += ((g_q.sigma * c_bar)/(SQRT_2_PI)) * (ERF(-mu_bar/sqrt_2_sig) - ERF((s - mu_bar)/sqrt_2_sig));
    }
    return _exp(T);
}

f32 simd_transmittance(const vec4f_t _o, const vec4f_t _n, const f32 s, const gaussians_t &gaussians)
{
    simd::Vec<simd::Float> T = simd::set1(0.f);
    for (u64 i = 0; i < gaussians.gaussians.size(); i += SIMD_FLOATS)
    {
        simd_gaussian_t g_q{
            .albedo{},
            .mu{ .x = simd::load(gaussians.gaussians_broadcast->mu.x + i),
                .y = simd::load(gaussians.gaussians_broadcast->mu.y + i),
                .z = simd::load(gaussians.gaussians_broadcast->mu.z + i) },
            .sigma = simd::load(gaussians.gaussians_broadcast->sigma + i),
            .magnitude = simd::load(gaussians.gaussians_broadcast->magnitude + i)
        };
        simd_vec4f_t o = simd_vec4f_t::from_vec4f_t(_o);
        simd_vec4f_t n = simd_vec4f_t::from_vec4f_t(_n);
        const simd_vec4f_t center_to_origin = g_q.mu - o;
        const simd::Vec<simd::Float> mu_bar = (center_to_origin).dot(n);
        const simd::Vec<simd::Float> c_bar = g_q.magnitude * SIMD_EXP(-((center_to_origin).sqnorm() - (mu_bar * mu_bar))/(simd::set1(2.f) * g_q.sigma * g_q.sigma));

        const simd::Vec<simd::Float> sqrt_2_sig = simd::set1(SQRT_2) * g_q.sigma;
        T += ((g_q.sigma * c_bar)/simd::set1(SQRT_2_PI)) * (SIMD_ERF(-mu_bar/sqrt_2_sig) - SIMD_ERF((simd::set1(s) - mu_bar)/sqrt_2_sig));
    }
    return EXP(simd::hadds(T));
}

simd::Vec<simd::Float> broadcast_transmittance(const simd_vec4f_t &o, const simd_vec4f_t &n, const simd::Vec<simd::Float> &s, const gaussians_t &gaussians)
{
    simd::Vec<simd::Float> T = simd::set1(0.f);
    for (u64 i = 0; i < gaussians.gaussians.size(); ++i)
    {
        const simd_gaussian_t G_q = simd_gaussian_t::from_gaussian_t(gaussians.gaussians[i]);
        const simd_vec4f_t center_to_origin = G_q.mu - o;
        const simd::Vec<simd::Float> mu_bar = (center_to_origin).dot(n);
        const simd::Vec<simd::Float> c_bar = G_q.magnitude * SIMD_EXP( -((center_to_origin).sqnorm() - (mu_bar * mu_bar))/(simd::set1(2.f) * G_q.sigma * G_q.sigma) );

        const simd::Vec<simd::Float> sqrt_2_sig = simd::set1(SQRT_2) * G_q.sigma;

        const simd::Vec<simd::Float> erf1 = SIMD_ERF(-mu_bar/sqrt_2_sig);
        const simd::Vec<simd::Float> erf2 = SIMD_ERF((s-mu_bar)/sqrt_2_sig);
        T += ((G_q.sigma * c_bar)/simd::set1(SQRT_2_PI)) * (erf1 - erf2);
    }
    return SIMD_EXP(T);
}

f32 transmittance_step(const vec4f_t o, const vec4f_t n, const f32 s, const f32 delta, const std::vector<gaussian_t> gaussians)
{
    f32 T = 0.f;
    for (f32 t = 0; t <= s; t += delta)
    {
        for (const gaussian_t &g : gaussians)
            T += delta * g.pdf(o + n * t);
    }
    return EXP(-T);
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
            f32 T = TRANSMITTANCE(o, n, s, gaussians);
            inner += G_q.pdf(o + (n * s)) * T * lambda_q;
        }
        L_hat = L_hat + (G_q.albedo * inner);
        //fmt::println("{}, {}, {}, {}", L_hat.x, L_hat.y, L_hat.z, L_hat.w);
    }
    return L_hat;
}

simd_vec4f_t simd_l_hat(const simd_vec4f_t o, const simd_vec4f_t n, const gaussians_t &gaussians)
{
    simd_vec4f_t L_hat{ .x = simd::set1(0.f), .y = simd::set1(0.f), .z = simd::set1(0.f) };
    for (const gaussian_t &_G_q : gaussians.gaussians)
    {
        const simd_gaussian_t G_q = simd_gaussian_t::from_gaussian_t(_G_q);
        const simd::Vec<simd::Float> lambda_q = G_q.sigma;
        simd::Vec<simd::Float> inner = simd::set1(0.f);
        for (i8 k = -4; k <= 0; ++k)
        {
            const simd::Vec<simd::Float> s = (G_q.mu - o).dot(n) + simd::set1((f32)k) * lambda_q;
            simd::Vec<simd::Float> T;
            T = broadcast_transmittance(o, n, s, gaussians);
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

bool simd_render_image(const u32 width, const u32 height, u32 *image, const i32 *bg_image, const f32 *xs, const f32 *ys, const gaussians_t &gaussians, const bool &running)
{
    static bool is_wayland_display = getenv("WAYLAND_DISPLAY") != NULL;
    static const simd_vec4f_t simd_origin = simd_vec4f_t::from_vec4f_t(vec4f_t{ 0.f, 0.f, 0.f });

    for (u64 i = 0; i < width * height; i += SIMD_FLOATS)
    {
        simd_vec4f_t dir = simd_vec4f_t{ .x = simd::load(xs + i), .y = simd::load(ys + i), .z = simd::set1(1.f) } - simd_origin;
        dir.normalize();
        const simd_vec4f_t color = simd_l_hat(simd_origin, dir, gaussians);
        const simd::Vec<simd::Int> bg = simd::load(bg_image + i);
        const simd::Vec<simd::Int> A = simd::set1<simd::Int>(0xFF000000);
        const simd::Vec<simd::Int> R = simd::cvts<simd::Int>((simd::set1(1.f) - color.w) * simd::cvts<simd::Float>(simd::srli<16>(bg & simd::set1(0x00FF0000))) + simd::min(color.x, simd::set1(1.f)) * simd::set1(255.f));
        const simd::Vec<simd::Int> G = simd::cvts<simd::Int>((simd::set1(1.f) - color.w) * simd::cvts<simd::Float>(simd::srli<8>(bg & simd::set1(0x0000FF00))) + color.y * simd::set1(255.f));
        const simd::Vec<simd::Int> B = simd::cvts<simd::Int>((simd::set1(1.f) - color.w) * simd::cvts<simd::Float>(bg & simd::set1(0x000000FF)) + color.z * simd::set1(255.f));
        if (is_wayland_display)
            simd::store((i32*)image + i, (A | R | simd::slli<8>(G) | simd::slli<16>(B)));
        else
            simd::store((i32*)image + i, (A | simd::slli<16>(R) | simd::slli<8>(G) | B));
        if (!running) return true;
    }
    return false;
}

/// NOTE: Tiles need to be a multiple of SIMD_FLOAT pixels wide
bool simd_render_image(const u32 width, const u32 height, u32 *image, const i32 *bg_image, const f32 *xs, const f32 *ys, const tiles_t &tiles, const bool &running)
{
    assert(tiles.w % SIMD_FLOATS == 0);
    static bool is_wayland_display = getenv("WAYLAND_DISPLAY") != NULL;
    static const simd_vec4f_t simd_origin = simd_vec4f_t::from_vec4f_t(vec4f_t{ 0.f, 0.f, 0.f });
    u64 tidx = 0;

    for (u64 i = 0; i < width * height; i += SIMD_FLOATS)
    {
        tidx = u64((xs[i] + 1.f)/tiles.tw) + u64((ys[i] + 1.f)/tiles.th) * tiles.w;
        simd_vec4f_t dir = simd_vec4f_t{ .x = simd::load(xs + i), .y = simd::load(ys + i), .z = simd::set1(1.f) } - simd_origin;
        dir.normalize();
        const simd_vec4f_t color = simd_l_hat(simd_origin, dir, tiles.gaussians[tidx]);
        simd::Vec<simd::Int> bg = simd::load(bg_image + i);
        simd::Vec<simd::Int> A = simd::set1<simd::Int>(0xFF000000);
        simd::Vec<simd::Int> R = simd::cvts<simd::Int>((simd::set1(1.f) - color.w) * simd::cvts<simd::Float>(simd::srli<16>(bg & simd::set1(0x00FF0000))) + color.x * simd::set1(255.f));
        simd::Vec<simd::Int> G = simd::cvts<simd::Int>((simd::set1(1.f) - color.w) * simd::cvts<simd::Float>(simd::srli<8>(bg & simd::set1(0x0000FF00))) + color.y * simd::set1(255.f));
        simd::Vec<simd::Int> B = simd::cvts<simd::Int>((simd::set1(1.f) - color.w) * simd::cvts<simd::Float>(bg & simd::set1(0x000000FF)) + color.z * simd::set1(255.f));
        if (is_wayland_display)
            simd::store((i32*)image + i, (A | R | simd::slli<8>(G) | simd::slli<16>(B)));
        else
            simd::store((i32*)image + i, (A | simd::slli<16>(R) | simd::slli<8>(G) | B));
        if (!running) return true;
    }
    return false;
}
