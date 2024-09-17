#!/usr/bin/env sh

rm -rf hpctoolkit
mkdir hpctoolkit
# spack load hpctoolkit

EVENTS="--event PAPI_TOT_CYC --event PAPI_TOT_INS --event PAPI_VEC_INS --event CPUTIME"
EVENTS="--event perf::L1-DCACHE-LOADS ${EVENTS}"
EVENTS="--event perf::L1-DCACHE-LOAD-MISSES ${EVENTS}"
EVENTS="--event perf::L1-ICACHE-LOADS ${EVENTS}"
EVENTS="--event perf::L1-ICACHE-LOAD-MISSES ${EVENTS}"

# HW cache evenets
EVENTS="--event perf::CACHE-REFERENCES ${EVENTS}"
EVENTS="--event perf::CACHE-MISSES ${EVENTS}"

run_test () {
    first_mode=5
    make clean &> /dev/null
    clang=""
    svml=""
    name="$1"
    if [ "$2" = "true" ]; then
        svml="--with-svml"
        name="${1}-svml"
        first_mode=6
    fi
    if [ "$1" = "gcc" ]; then
        clang="--use-gcc"
    fi
    make build -j32 ARGS="${clang} ${svml}" &> /dev/null
    for mode in $(seq $first_mode 8); do
        rm -rf hpctoolkit-volumetric-ray-tracer-measurements
        HPCRUN_TRACE=1 hpcrun $EVENTS -- ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/teapot.obj -t32 --tiles 16 --frames 5 -m "${mode}" > /dev/null
        hpcstruct hpctoolkit-volumetric-ray-tracer-measurements > /dev/null
        hpcprof hpctoolkit-volumetric-ray-tracer-measurements > /dev/null
        mv hpctoolkit-volumetric-ray-tracer-database hpctoolkit/hpctoolkit-volumetric-ray-tracer-database-"${name}"-mode-"${mode}"-mt
    done
}

if [ "$1" = "long" ]; then
    run_test gcc false
    run_test gcc true
    run_test clang true
fi
run_test clang false
