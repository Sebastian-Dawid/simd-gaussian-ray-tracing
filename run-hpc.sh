#!/usr/bin/zsh

rm -rf hpctoolkit
mkdir hpctoolkit
spack load hpctoolkit

rm runtime-log
touch runtime-log

# GCC
echo "GCC:" >> runtime-log
make clean
make build -j32
for mode in $(seq 8); do
    rm -rf hpctoolkit-volumetric-ray-tracer-measurements
    echo -n "\tMODE $mode ST: " >> runtime-log
    HPCRUN_TRACE=1 hpcrun --event PAPI_TOT_CYC --event PAPI_TOT_INS --event PAPI_VEC_INS --event CPU-CYCLES --event CPUTIME --event INSTRUCTIONS -- ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj -t1 --tiles 16 -m $mode >> runtime-log
    hpcstruct hpctoolkit-volumetric-ray-tracer-measurements
    hpcprof hpctoolkit-volumetric-ray-tracer-measurements
    mv hpctoolkit-volumetric-ray-tracer-database hpctoolkit/hpctoolkit-volumetric-ray-tracer-database-gcc-mode-$mode-st

    if [[ $mode -gt 4 ]]; then
        rm -rf hpctoolkit-volumetric-ray-tracer-measurements
        echo -n "\tMODE $mode MT(32): " >> runtime-log
        HPCRUN_TRACE=1 hpcrun --event PAPI_TOT_CYC --event PAPI_TOT_INS --event PAPI_VEC_INS --event CPU-CYCLES --event CPUTIME --event INSTRUCTIONS -- ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj -t32 --tiles 16 -m $mode >> runtime-log
        hpcstruct hpctoolkit-volumetric-ray-tracer-measurements
        hpcprof hpctoolkit-volumetric-ray-tracer-measurements
        mv hpctoolkit-volumetric-ray-tracer-database hpctoolkit/hpctoolkit-volumetric-ray-tracer-database-gcc-mode-$mode-mt
    fi
done

# CLANG
echo "CLANG:" >> runtime-log
make clean
make build -j32 ARGS="--with-clang"
for mode in $(seq 8); do
    rm -rf hpctoolkit-volumetric-ray-tracer-measurements
    echo -n "\tMODE $mode ST: " >> runtime-log
    HPCRUN_TRACE=1 hpcrun --event PAPI_TOT_CYC --event PAPI_TOT_INS --event PAPI_VEC_INS --event CPU-CYCLES --event CPUTIME --event INSTRUCTIONS -- ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj -t1 --tiles 16 -m $mode >> runtime-log
    hpcstruct hpctoolkit-volumetric-ray-tracer-measurements
    hpcprof hpctoolkit-volumetric-ray-tracer-measurements
    mv hpctoolkit-volumetric-ray-tracer-database hpctoolkit/hpctoolkit-volumetric-ray-tracer-database-clang-mode-$mode-st

    if [[ $mode -gt 4 ]]; then
        rm -rf hpctoolkit-volumetric-ray-tracer-measurements
        echo -n "\tMODE $mode MT(32): " >> runtime-log
        HPCRUN_TRACE=1 hpcrun --event PAPI_TOT_CYC --event PAPI_TOT_INS --event PAPI_VEC_INS --event CPU-CYCLES --event CPUTIME --event INSTRUCTIONS -- ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj -t32 --tiles 16 -m $mode >> runtime-log
        hpcstruct hpctoolkit-volumetric-ray-tracer-measurements
        hpcprof hpctoolkit-volumetric-ray-tracer-measurements
        mv hpctoolkit-volumetric-ray-tracer-database hpctoolkit/hpctoolkit-volumetric-ray-tracer-database-clang-mode-$mode-mt
    fi
done
