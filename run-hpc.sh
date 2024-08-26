#!/usr/bin/env sh

rm -rf hpctoolkit
mkdir hpctoolkit
# spack load hpctoolkit

rm hpc-runtimes.log
touch hpc-runtimes.log

EVENTS="--event PAPI_TOT_CYC --event PAPI_TOT_INS --event PAPI_VEC_INS --event CPUTIME --event MEMLEAK"

run_test () {
    first_mode=1
    make clean
    clang=""
    svml=""
    name="$1"
    if [ "$2" = "true" ]; then
        echo "${1}-svml:" >> hpc-runtimes.log
        first_mode=2
        svml="--with-svml"
        name="${1}-svml"
    else
        echo "$1:" >> hpc-runtimes.log
    fi
    if [ "$1" = "clang" ]; then
        clang="--with-clang"
    fi
    make build -j32 ARGS="${clang} ${svml}"
    for mode in $(seq $first_mode 8); do
        rm -rf hpctoolkit-volumetric-ray-tracer-measurements
        printf "\tMODE %s ST: " "${mode}" >> hpc-runtimes.log
        HPCRUN_TRACE=1 hpcrun $EVENTS  -- ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj -t1 --tiles 16 -m "${mode}" >> hpc-runtimes.log
        hpcstruct hpctoolkit-volumetric-ray-tracer-measurements
        hpcprof hpctoolkit-volumetric-ray-tracer-measurements
        mv hpctoolkit-volumetric-ray-tracer-database hpctoolkit/hpctoolkit-volumetric-ray-tracer-database-"${name}"-mode-"${mode}"-st

        if [ "${mode}" -gt 4 ]; then
            rm -rf hpctoolkit-volumetric-ray-tracer-measurements
            printf "\tMODE %s MT(32): " "${mode}" >> hpc-runtimes.log
            HPCRUN_TRACE=1 hpcrun $EVENTS -- ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj -t32 --tiles 16 -m "${mode}" >> hpc-runtimes.log
            hpcstruct hpctoolkit-volumetric-ray-tracer-measurements
            hpcprof hpctoolkit-volumetric-ray-tracer-measurements
            mv hpctoolkit-volumetric-ray-tracer-database hpctoolkit/hpctoolkit-volumetric-ray-tracer-database-"${name}"-mode-"${mode}"-mt
        fi
    done
}

run_test gcc false
run_test gcc true
run_test clang false
run_test clang true
