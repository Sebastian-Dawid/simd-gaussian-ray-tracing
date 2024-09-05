#!/usr/bin/env sh

rm runtimes.log

run_test () {
    make clean
    clang=""
    svml=""
    if [ "$2" = "true" ]; then
        echo "${1}-svml:" >> runtimes.log
        svml="--with-svml"
    else
        echo "$1:" >> runtimes.log
    fi
    if [ "$1" = "gcc" ]; then
        clang="--use-gcc"
    fi
    make build -j32 ARGS="${clang} ${svml}"
    for mode in $(seq 1 8); do
        frames=10
        if [ "${mode}" -eq 5 ] || [ "${mode}" -eq 1 ]; then
            frames=1
        fi
        printf "\tMODE %s ST: " "${mode}" >> runtimes.log
        ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj --frames "${frames}" -t1 --tiles 16 -m "${mode}" >> runtimes.log

        if [ "${mode}" -gt 4 ]; then
            printf "\tMODE %s MT(32): " "${mode}" >> runtimes.log
            ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj --frames "${frames}" -t32 --tiles 16 -m "${mode}" >> runtimes.log
        fi
    done
}

run_test gcc false
if [ $1 = "long" ]; then
    run_test gcc true
fi
run_test clang false
run_test clang true
