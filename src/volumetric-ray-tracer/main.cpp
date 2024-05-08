#include "external/imgui/backend/imgui_impl_glfw.h"
#include "external/imgui/backend/imgui_impl_vulkan.h"
#include <cmath>
#include <cstdlib>
#include <fmt/core.h>
#include <thread>
#include <vk-renderer/vk-renderer.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <include/stb_image_write.h>

#pragma GCC diagnostic pop

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
};

struct gaussian_t
{
    vec4f_t albedo;
    vec4f_t mu;
    float sigma;
    float magnitude;
    float pdf(vec4f_t x) const
    {
        return this->magnitude * std::exp(-((x - this->mu).dot(x - this->mu))/(2 * std::pow(this->sigma, 2.f)));
    }
};

static float transmittance(const vec4f_t o, const vec4f_t n, const float s, const std::vector<gaussian_t> gaussians)
{
    float T = 0.f;
    for (const gaussian_t &g_q : gaussians)
    {
        const float mu_bar = (g_q.mu - o).dot(n);
        const float c_bar = g_q.magnitude * std::exp(-((g_q.mu - o).dot(g_q.mu - o) - std::pow(mu_bar, 2.f))/(2*std::pow(g_q.sigma, 2.f)));
        T += ((g_q.sigma * c_bar)/(std::sqrt(2/M_PIf))) * (std::erf(-mu_bar/(std::sqrt(2) * g_q.sigma)) - std::erf((s - mu_bar)/(std::sqrt(2) * g_q.sigma)));
    }
    return std::exp(T);
}

static float transmittance_step(const vec4f_t o, const vec4f_t n, const float s, const float delta, const std::vector<gaussian_t> gaussians)
{
    float T = 0.f;
    for (float t = 0; t <= s; t += delta)
    {
        for (const gaussian_t &g : gaussians)
            T += delta * g.pdf(o + n * t);
    }
    return std::exp(-T);
}

static bool approx_transmittance = false;
static bool new_approx_transmittance = approx_transmittance;

static vec4f_t l_hat(const vec4f_t o, const vec4f_t n, const std::vector<gaussian_t> gaussians)
{
    vec4f_t L_hat{ .x = 0.f, .y = 0.f, .z = 0.f };
    for (const gaussian_t &G_q : gaussians)
    {
        const float lambda_q = G_q.sigma;
        float inner = 0.f;
        for (i8 k = -4; k <= 0; ++k)
        {
            const float s = (G_q.mu - o).dot(n) + k * lambda_q;
            float T;
            if (approx_transmittance)
                T = transmittance_step(o, n, s, lambda_q, gaussians);
            else
                T = transmittance(o, n, s, gaussians);
            inner += G_q.pdf(o + (n * s)) * T * lambda_q;
        }
        L_hat = L_hat + (G_q.albedo * inner);
    }
    return L_hat;
}

int main(i32 argc, char **argv)
{
    u64 width = 256, height = 256;
    if (argc >= 3)
    {
        width = strtoul(argv[1], NULL, 10);
        height = strtoul(argv[2], NULL, 10);
    }
    u64 new_width = width, new_height = height;
    
    std::vector<gaussian_t> gaussians = { gaussian_t{ .albedo{ 0.f, 1.f, 0.f, .1f }, .mu{ .3f, .3f, .5f }, .sigma = 0.1f, .magnitude = 5.f }, gaussian_t{ .albedo{ 0.f, 0.f, 1.f, .7f }, .mu{ -.3f, -.3f, 0.f }, .sigma = 0.4f, .magnitude = 2.f }, gaussian_t{ .albedo{ 1.f, 0.f, 0.f, 1.f }, .mu{ 0.f, 0.f, 2.f }, .sigma = .75f, .magnitude = 1.f } };

    renderer_t renderer;
    if (!renderer.init(width, height, "Test")) return EXIT_FAILURE;
    bool running = true;
    std::thread render_thread([&](){
            while (!glfwWindowShouldClose(renderer.window.ptr))
            {
                glfwPollEvents();
                if (glfwGetKey(renderer.window.ptr, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(renderer.window.ptr, GLFW_TRUE);
                
                {
                    ImGui_ImplVulkan_NewFrame();
                    ImGui_ImplGlfw_NewFrame();
                    ImGui::NewFrame();

                    ImGui::Begin("Gaussians");
                    ImGui::InputFloat3("mu", &gaussians[2].mu.x);
                    ImGui::InputInt("width", (int*)&new_width);
                    ImGui::InputInt("height", (int*)&new_height);
                    ImGui::Checkbox("approx", &new_approx_transmittance);
                    ImGui::End();

                    ImGui::Render();
                }
                
                if (!renderer.update())
                {
                    return;
                }
            }
            running = false;
        });

    u32 *image = (u32*)std::calloc(width * height, sizeof(u32));

    const vec4f_t origin = { 0.f, 0.f, -5.f };
    while (running)
    {
        if (new_width != width || new_height != height)
        {
            width = new_width;
            height = new_height;
            image = (u32*)std::realloc(image, width * height * sizeof(u32));
        }
        approx_transmittance = new_approx_transmittance;
        vec4f_t pt = { -1.f, -1.f, 0.f };
        for (u64 y = 0; y < height; ++y)
        {
            for (u64 x = 0; x < width; ++x)
            {
                vec4f_t dir = pt - origin;
                dir.normalize();
                if (x == width/2 && y == height/2)
                {
                    FILE *CSV = std::fopen("data.csv", "wd");
                    fmt::println(CSV, "s, T1, T2");
                    for (float k = -4.f; k <= 0; k += .5f)
                    {
                        float s = (gaussians[2].mu - origin).dot(dir) + k * gaussians[2].sigma;
                        fmt::println(CSV, "{}, {}, {}", s, transmittance(origin, dir, s, gaussians), transmittance_step(origin, dir, s, gaussians[2].sigma, gaussians));
                    }
                    std::fclose(CSV);
                }
                vec4f_t color = l_hat(origin, dir, gaussians);
                u32 A = 0xFF000000; // final alpha channel is always 1
                u32 R = (u32)(color.x * 255);
                u32 G = (u32)(color.y * 255);
                u32 B = (u32)(color.z * 255);
                image[y * width + x] = A | R << 16 | G << 8 | B;
                pt.x += 1/(width/2.f);
                if (!running) goto cleanup;
            }
            pt.x = -1.f;
            pt.y += 1/(height/2.f);
        }

        if (argc >= 4)
            stbi_write_png(argv[3], width, height, 4, image, width * 4);
        renderer.stage_image(image, width, height);
    }

cleanup:
    render_thread.join();
    free(image);

    return EXIT_SUCCESS;
}
