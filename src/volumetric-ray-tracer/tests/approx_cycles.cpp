#include <include/definitions.h>
#include <include/error_fmt.h>

#include <vector>
#define INLINE_VRT_APPROX
#include <vrt/approx.h>

#include <include/TimeMeasurement.H>
#include <sched.h>

#include <rdpmc.h>

#ifndef ITER
#define ITER 10000
#endif

#ifndef COUNT
#define COUNT 2048
#endif

#ifndef SEED
#define SEED 42
#endif

using namespace vrt;
using namespace approx;

FILE *dev_null = std::fopen("/dev/null", "wd");

template<u64 Iter, u64 Step, u64 Size, void (*Fn)(f32*, f32*)>
inline u64 benchmark(struct rdpmc_ctx* ctx, std::vector<f32, simd_aligned_allocator<f32, SIMD_BYTES>> in)
{
    u64 start = 0, end = 0, acc = 0;
    std::vector<f32, simd_aligned_allocator<f32, SIMD_BYTES>> out(Step);
    for (u64 i = 0; i < Iter; ++i)
    {
        for (u64 j = 0; j < in.size(); j += Step)
        {
            f32* _in = in.data() + j;
            f32* _out = out.data();
            for (u64 k = 0; k < Step; k += Size)
            {
                start = rdpmc_read(ctx);
                asm volatile("":::"memory");
                Fn(_in + k, _out + k);
                asm volatile("":::"memory");
                end = rdpmc_read(ctx);
                acc += end - start;
                fmt::print(dev_null, "{}\n", _out[k]);
            }
            for (u64 _ = 0; _ < Step; ++_) fmt::print(dev_null, "{}\n", _out[_]);
        }
    }
    return acc;
}

i32 main()
{
    cpu_set_t mask;
    CPU_ZERO(&mask);    
    CPU_SET(2, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
    FILE *CSV = std::fopen("csv/simd_erf.csv", "wd");
    fmt::print(CSV, "count,t_simd_spline,t_simd_spline_mirror,t_simd_abramowitz,t_simd_taylor,t_svml,t_spline,t_spline_mirror,t_abramowitz,t_taylor,t_std\n");
    struct rdpmc_ctx ctx;

    srand48(SEED);

    if (rdpmc_open(PERF_COUNT_HW_CPU_CYCLES, &ctx) < 0) exit(EXIT_FAILURE);
    {
        std::vector<f32, simd_aligned_allocator<f32, SIMD_BYTES>> in(SIMD_FLOATS * COUNT);
        for (u64 j = 0; j < SIMD_FLOATS * COUNT; ++j)
            in[j] = -6.f + 12.f * drand48();

        f32 t_svml            = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, SIMD_FLOATS, [](f32* in, f32* out) -> void { simd::store(out, svml_erf(simd::load(in))); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        f32 t_simd            = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, SIMD_FLOATS, [](f32* in, f32* out) -> void { simd::store(out, simd_spline_erf(simd::load(in))); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        f32 t_simd_mirror     = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, SIMD_FLOATS, [](f32* in, f32* out) -> void { simd::store(out, simd_spline_erf_mirror(simd::load(in))); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        f32 t_simd_abramowitz = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, SIMD_FLOATS, [](f32* in, f32* out) -> void { simd::store(out, simd_abramowitz_stegun_erf(simd::load(in))); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        f32 t_simd_taylor     = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, SIMD_FLOATS, [](f32* in, f32* out) -> void { simd::store(out, simd_taylor_erf(simd::load(in))); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);

        f32 t_spline        = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, 1, [](f32* in, f32* out) -> void { *out = spline_erf(*in); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        f32 t_spline_mirror = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, 1, [](f32* in, f32* out) -> void { *out = spline_erf_mirror(*in); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        f32 t_abramowitz    = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, 1, [](f32* in, f32* out) -> void { *out = abramowitz_stegun_erf(*in); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        f32 t_taylor        = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, 1, [](f32* in, f32* out) -> void { *out = taylor_erf(*in); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        f32 t_std           = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, 1, [](f32* in, f32* out) -> void { *out = erff(*in); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);

        fmt::print(CSV, "{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}\n", SIMD_FLOATS * COUNT, t_simd, t_simd_mirror, t_simd_abramowitz, t_simd_taylor, t_svml, t_spline, t_spline_mirror, t_abramowitz, t_taylor, t_std);
    }
    std::fclose(CSV);

    CSV = std::fopen("csv/simd_exp.csv", "wd");
    fmt::print(CSV, "count,t_simd,t_simd_fast,t_svml,t_vcl,t_spline,t_fast,t_std\n");
    {
        std::vector<f32, simd_aligned_allocator<f32, SIMD_BYTES>> in(SIMD_FLOATS * COUNT);
        for (u64 j = 0; j < SIMD_FLOATS * COUNT; ++j)
            in[j] = -10.f + 10.f * drand48();

        f32 t_svml      = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, SIMD_FLOATS, [](f32* in, f32* out) -> void { simd::store(out, svml_exp(simd::load(in))); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        f32 t_vcl       = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, SIMD_FLOATS, [](f32* in, f32* out) -> void { simd::store(out, vcl_exp(simd::load(in))); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        f32 t_simd      = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, SIMD_FLOATS, [](f32* in, f32* out) -> void { simd::store(out, simd_spline_exp(simd::load(in))); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        f32 t_simd_fast = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, SIMD_FLOATS, [](f32* in, f32* out) -> void { simd::store(out, simd_fast_exp(simd::load(in))); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        
        f32 t_spline    = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, 1, [](f32* in, f32* out) -> void { *out = spline_exp(*in); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        f32 t_fast      = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, 1, [](f32* in, f32* out) -> void { *out = fast_exp(*in); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);
        f32 t_std       = static_cast<f32>(benchmark<ITER, SIMD_FLOATS, 1, [](f32* in, f32* out) -> void { *out = expf(*in); }>(&ctx, in))/(ITER * COUNT * SIMD_FLOATS);

        fmt::print(CSV, "{}, {}, {}, {}, {}, {}, {}, {}\n", SIMD_FLOATS * COUNT, t_simd, t_simd_fast, t_svml, t_vcl, t_spline, t_fast, t_std);
    }
    std::fclose(CSV);

    char *args[] = { (char*)"./julia/wrapper.sh", (char*)"./julia/simd_erf_timing.jl", NULL };
    execvp("./julia/wrapper.sh", args);
    fmt::print(stderr, "[ {} ]\tGenerating Plots failed with error: {}\n", ERROR_FMT("ERROR"), strerror(errno));
    exit(EXIT_SUCCESS);
}
