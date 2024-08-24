#!/usr/bin/env sh

rm -rf hpctoolkit
mkdir hpctoolkit
# spack load hpctoolkit

rm runtime-log
touch runtime-log

EVENTS="--event PAPI_TOT_CYC --event PAPI_TOT_INS --event PAPI_VEC_INS --event CPUTIME --event MEMLEAK"

run_test () {
    mode_count=8
    offset=0
    make clean
    clang=""
    svml=""
    name="$1"
    if [ "$2" = "true" ]; then
        echo "${1}-svml:" >> runtime-log
        mode_count=7
        offset=1
        svml="--with-svml"
        name="${1}-svml"
    else
        echo "$1:" >> runtime-log
    fi
    if [ "$1" = "clang" ]; then
        clang="--with-clang"
    fi
    make build -j32 ARGS="${clang} ${svml}"
    for mode in $(seq $mode_count); do
        rm -rf hpctoolkit-volumetric-ray-tracer-measurements
        printf "\tMODE %s ST: " $((mode + offset)) >> runtime-log
        HPCRUN_TRACE=1 hpcrun $EVENTS  -- ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj -t1 --tiles 16 -m $((mode + offset)) >> runtime-log
        hpcstruct hpctoolkit-volumetric-ray-tracer-measurements
        hpcprof hpctoolkit-volumetric-ray-tracer-measurements
        mv hpctoolkit-volumetric-ray-tracer-database hpctoolkit/hpctoolkit-volumetric-ray-tracer-database-"${name}"-mode-$((mode + offset))-st

        if [ $((mode + offset)) -gt 4 ]; then
            rm -rf hpctoolkit-volumetric-ray-tracer-measurements
            printf "\tMODE %s MT(32): " $((mode + offset)) >> runtime-log
            HPCRUN_TRACE=1 hpcrun $EVENTS -- ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj -t32 --tiles 16 -m $((mode + offset)) >> runtime-log
            hpcstruct hpctoolkit-volumetric-ray-tracer-measurements
            hpcprof hpctoolkit-volumetric-ray-tracer-measurements
            mv hpctoolkit-volumetric-ray-tracer-database hpctoolkit/hpctoolkit-volumetric-ray-tracer-database-"${name}"-mode-$((mode + offset))-mt
        fi
    done
}

run_test gcc false
run_test gcc true
run_test clang false
run_test clang true
