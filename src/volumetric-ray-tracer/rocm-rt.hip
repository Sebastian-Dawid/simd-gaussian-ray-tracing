#include <hip/hip_runtime.h>
#include <include/definitions.h>
#include <include/TimeMeasurement.H>
#include <vector>
#ifdef WITH_RENDERER
#include "imgui.h"
#include <vk-renderer/vk-renderer.h>
#endif

#ifndef GRID
#define GRID 0
#endif

#define SIGN(x) ((x >= 0) - (x < 0))
#define HIP_CHECK(condition)                        \
{                                                   \
    const hipError_t error = condition;             \
    if (error != hipSuccess)                        \
    {                                               \
        fprintf(stderr, "ERROR: %s at %s:%d\n",     \
                hipGetErrorString(error),           \
                __FILE__, __LINE__);                \
        exit(EXIT_FAILURE);                         \
    }                                               \
}

#define GENERATE_PROJECTION_PLANE(XS, YS, W, H)                         \
{                                                                       \
    XS = (f32*)malloc(sizeof(f32) * W * H);                             \
    YS = (f32*)malloc(sizeof(f32) * W * H);                             \
    for (u64 i = 0; i < H; ++i)                                         \
    {                                                                   \
        for (u64 j = 0; j < W; ++j)                                     \
        {                                                               \
            xs[i * W + j] = -1.f + j/(W/2.f);                           \
            ys[i * W + j] = -1.f + i/(H/2.f);                           \
        }                                                               \
    }                                                                   \
}

static constexpr f32 a = (1 << 23)/0.69312471805599453f;
static constexpr f32 b = (1 << 23) * (127 - 0.043677448f);
static constexpr f32 c = (1 << 23);
static constexpr f32 d = (1 << 23) * 255;
__device__ f32 fast_exp(f32 x)
{
    x = a *x + b;
    if (x < c || x > d)
        x = (x < c) ? 0.f : d;

    u32 n = static_cast<u32>(x);
    return *(f32*)&n;
}

__device__ f32 abramowitz_stegun_erf(f32 x)
{
    f32 sign = SIGN(x);
    x *= sign;
    const f32 square = x*x;
    constexpr f32 a[] = { 0.278393f, 0.230389f, 0.000972f, 0.078108f };
    const f32 denom = 1 + a[0]*x + a[1]*square + a[2]*square*x + a[3]*square*square;
    const f32 val = 1 - 1/(denom*denom*denom*denom);
    return val * sign;
}

struct vec3f_t
{
    f32 x, y, z;
    __device__ vec3f_t operator+(const vec3f_t &other) const
    {
        return {this->x + other.x, this->y + other.y, this->z + other.z};
    }
    __device__ vec3f_t operator-(const vec3f_t &other) const
    {
        return {this->x - other.x, this->y - other.y, this->z - other.z};
    }
    __device__ vec3f_t operator*(const f32 &lambda) const
    {
        return {this->x * lambda, this->y * lambda, this->z * lambda};
    }
    __device__ f32 dot(const vec3f_t &other) const
    {
        return this->x * other.x + this->y * other.y + this->z * other.z;
    }
    __device__ void normalize()
    {
        f32 norm = sqrt(this->dot(*this));
        this->x /= norm;
        this->y /= norm;
        this->z /= norm;
    }
};

struct gaussian_t
{
    vec3f_t albedo;
    vec3f_t mu;
    f32 sigma;
    f32 magnitude;
    __device__ f32 pdf(vec3f_t x) const
    {
        return this->magnitude * fast_exp(-((x - this->mu).dot(x - this->mu))/(2.f * this->sigma * this->sigma));
    }
};

constexpr f32 SQRT_2_PI = 0.7978845608028654f;
constexpr f32 SQRT_2 = 1.4142135623730951f;

template<u32 block_size>
__global__ void render(const f32 *xs, const f32 *ys, u32 *image, gaussian_t* gaussians, size_t num_gaussians)
{
    const u32 tx = threadIdx.x;
    const u32 ty = threadIdx.y;
    const u32 bx = blockIdx.x;
    const u32 by = blockIdx.y;

    
    const vec3f_t origin = { 0.f, 0.f, -5.f };

    const size_t idx = (by * block_size + ty) * (gridDim.x * block_size) + bx * block_size + tx;
    vec3f_t dir = { xs[idx] - origin.x, ys[idx] - origin.y, 0.f - origin.z };
    dir.normalize();

    __syncthreads();
    vec3f_t L_hat = {0.f, 0.f, 0.f};
    for (size_t i = 0; i < num_gaussians; ++i)
    {
        const f32 lambda_q = gaussians[i].sigma;
        f32 inner = 0.f;
        for (i8 k = -4; k <= 0; ++k)
        {
            const f32 s = (gaussians[i].mu - origin).dot(dir) + k * lambda_q;
            f32 T = 0.f;
            for (size_t j = 0; j < num_gaussians; ++j)
            {
                const f32 mu_bar = (gaussians[j].mu - origin).dot(dir);
                const f32 c_bar = gaussians[j].magnitude * fast_exp( -((gaussians[j].mu - origin).dot(gaussians[j].mu - origin) - mu_bar*mu_bar)/(2.f * gaussians[j].sigma * gaussians[j].sigma) );
                T += ((gaussians[j].sigma * c_bar)/(SQRT_2_PI)) * (abramowitz_stegun_erf(-mu_bar/(SQRT_2 * gaussians[j].sigma)) - abramowitz_stegun_erf((s - mu_bar)/(SQRT_2 * gaussians[j].sigma)));
            }
            inner += (gaussians[i].pdf(origin + (dir * s))) * fast_exp(T) * lambda_q;
        }
        L_hat = L_hat + (gaussians[i].albedo * inner);
    }

    constexpr u32 A = 0xFF000000;
    const u32 R = (u32)(L_hat.x * 255);
    const u32 G = (u32)(L_hat.y * 255);
    const u32 B = (u32)(L_hat.z * 255);
    const u32 color = A | R | G << 8 | B << 16;

    __syncthreads();
    image[idx] = color;
}

i32 main(i32 argc, char **argv)
{
    int device_id;
    hipGetDevice(&device_id);
    hipDeviceProp_t prop;
    hipGetDeviceProperties(&prop, device_id);
    printf("Running on: %s\n", prop.name);

    constexpr u32 W = 256;
    constexpr u32 H = 256;
    u32 image[W * H];
    u32 *hip_image;
    const size_t img_size = sizeof(u32) * W * H;
    HIP_CHECK(hipMalloc(&hip_image, img_size));
    f32 *xs, *ys;
    GENERATE_PROJECTION_PLANE(xs, ys, W, H);
    f32 *hip_xs, *hip_ys;
    HIP_CHECK(hipMalloc(&hip_xs, img_size));
    HIP_CHECK(hipMalloc(&hip_ys, img_size));
    HIP_CHECK(hipMemcpy(hip_xs, xs, img_size, hipMemcpyHostToDevice));
    HIP_CHECK(hipMemcpy(hip_ys, ys, img_size, hipMemcpyHostToDevice));

    double frame_time = 0.0;
#ifdef WITH_RENDERER
    renderer_t renderer;
    if (!renderer.init(W, H, "GPU Test")) return EXIT_FAILURE;
    renderer.custom_imgui = [&](){
        ImGui::Begin("Debug");
        ImGui::Text("frame time: %g ms", frame_time);
        ImGui::End();
    };    
    bool running = true;
    std::thread render_thread([&](){ renderer.run(running); });
#else
    bool running = false;
#endif
    std::vector<gaussian_t> gaussians;
    if (GRID)
    {
        constexpr u8 grid_dim = 8;
        for (u8 i = 0; i < grid_dim; ++i)
            for (u8 j = 0; j < grid_dim; ++j)
                gaussians.push_back(gaussian_t{
                        .albedo{ 1.f - (i * grid_dim + j)/(f32)(grid_dim*grid_dim), 0.f, 0.f + (i * grid_dim + j)/(f32)(grid_dim*grid_dim) },
                        .mu{ -1.f + 1.f/grid_dim + i * 1.f/(grid_dim/2.f), -1.f + 1.f/grid_dim + j * 1.f/(grid_dim/2.f), 0.f },
                        .sigma = 1.f/(2 * grid_dim),
                        .magnitude = 3.f
                        });
    }
    else
    {
        gaussians = { gaussian_t{ .albedo{ 0.f, 1.f, 0.f }, .mu{ .3f, .3f, .5f }, .sigma = 0.1f, .magnitude = 2.f }, gaussian_t{ .albedo{ 0.f, 0.f, 1.f }, .mu{ -.3f, -.3f, 0.f }, .sigma = 0.4f, .magnitude = .7f }, gaussian_t{ .albedo{ 1.f, 0.f, 0.f }, .mu{ 0.f, 0.f, 2.f }, .sigma = .75f, .magnitude = 1.f } };
    }
    gaussian_t *hip_gaussians;
    HIP_CHECK(hipMalloc(&hip_gaussians, sizeof(*hip_gaussians) * gaussians.size()));
    HIP_CHECK(hipMemcpy(hip_gaussians, gaussians.data(), sizeof(*hip_gaussians) * gaussians.size(), hipMemcpyHostToDevice));

    do
    {
        struct timespec start, end;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
        render<16><<<dim3(256/16, 256/16), dim3(16, 16), 0, hipStreamDefault>>>(hip_xs, hip_ys, hip_image, hip_gaussians, gaussians.size());
        HIP_CHECK(hipGetLastError());
        HIP_CHECK(hipMemcpy(image, hip_image, img_size, hipMemcpyDeviceToHost));
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
        frame_time = simd::timeSpecDiffNsec(end, start)/1000000.0;
        printf("frame time: %g\n", frame_time);
#ifdef WITH_RENDERER
        renderer.stage_image(image, W, H);
#endif
    } while(running);

#ifdef WITH_RENDERER
    render_thread.join();
#endif

    HIP_CHECK(hipFree(hip_gaussians));
    HIP_CHECK(hipFree(hip_image));
    HIP_CHECK(hipFree(hip_xs));
    HIP_CHECK(hipFree(hip_ys));
}
