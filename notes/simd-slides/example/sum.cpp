#include "../../../src/include/definitions.h"
#include "../../../src/include/tsimd.H"
#include <cstdlib>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define PAGE 4096

template<typename T>
struct circ_buf_t
{
    T *ptr = nullptr;
    size_t pages = 0;
    size_t repeat = 0;
    size_t len = 0;

    circ_buf_t(size_t pages, size_t repeat)
    {
        this->pages = pages;
        i32 fd = shm_open("/buf", O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd == -1)
        {
            return;
        }
        shm_unlink("/buf");

        size_t size = pages * PAGE;
        ftruncate(fd, size);
        
        i32 prot = PROT_READ | PROT_WRITE;
        this->ptr = static_cast<T*>(mmap(NULL, size, prot, MAP_SHARED, fd, 0));
        for (size_t i = 0; i < repeat; ++i)
        {
            T* old = this->ptr;
            this->ptr = static_cast<T*>(mmap(NULL, size, prot, MAP_SHARED, fd, 0));
            if (this->ptr == MAP_FAILED)
            {
                this->ptr = old;
                this->len = (this->pages * PAGE * this->repeat)/sizeof(T);
                return;
            }
            this->repeat++;
        }
        close(fd);
        this->len = (this->pages * PAGE * this->repeat)/sizeof(T);
    }
};

f64 dependant_sum(circ_buf_t<f64> &buf)
{
    simd::Vec<simd::Double> sum = simd::set1<simd::Double>(0.);
    for (size_t i = 0; i < buf.len; i += SIMD_FLOATS/2)
    {
        sum += simd::load(buf.ptr + i);
    }
    return simd::hadds(sum);
}

f64 sum(circ_buf_t<f64> &buf)
{
    simd::Vec<simd::Double> sum_a = simd::set1<simd::Double>(0.0), sum_b = simd::set1<simd::Double>(0.0), sum_c = simd::set1<simd::Double>(0.0), sum_d = simd::set1<simd::Double>(0.0);
    for (size_t i = 0; i < buf.len; i += 4 * SIMD_FLOATS/2)
    {
        sum_a += simd::load(buf.ptr + i);
        sum_b += simd::load(buf.ptr + i + SIMD_FLOATS/2);
        sum_c += simd::load(buf.ptr + i + SIMD_FLOATS);
        sum_d += simd::load(buf.ptr + i + SIMD_FLOATS/2 * 3);
    }
    return simd::hadds((sum_a + sum_b) + (sum_c + sum_d));
}

i32 main(i32 argc, char** argv)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(2, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    circ_buf_t<f64> buf(32, strtoul(argv[1], NULL, 10));
    for (size_t i = 0; i < buf.len/buf.repeat; ++i) buf.ptr[i] = static_cast<f64>(drand48());
    f64 avg = 0.f;
    for (size_t i = 0; i < 1024; ++i)
    {
        f64 a = dependant_sum(buf);
        f64 b = sum(buf);
        avg += (a + b)/2.f;
    }
    printf("%f\n", avg/1024.f);
    return EXIT_SUCCESS;
}
