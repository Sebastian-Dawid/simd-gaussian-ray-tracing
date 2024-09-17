#!/usr/bin/env sh

rm runtimes.log

run_test () {
    make clean &> /dev/null
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
    make build -j32 ARGS="${clang} ${svml}" &> /dev/null
    for mode in $(seq 5 8); do
        frames=1000
        if [ "${mode}" -eq 5 ] || [ "${mode}" -eq 1 ]; then
            if [ "$2" = "true" ]; then
                continue
            fi
            frames=100
        fi
        printf "\tMODE %s MT(32): " "${mode}" >> runtimes.log
        ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj --frames "${frames}" -t32 --tiles 16 -m "${mode}" >> runtimes.log
    done
    {
        printf "\tTILING BASELINE: "
        ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj -t1 -m 5
        printf "\tSEQ. BASELINE: "
        ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj -m 1
    } >> runtimes.log
}

{
    printf "START: "
    date
} >> runtimes.log

run_test gcc false
run_test clang false

if [ "$1" = "long" ]; then
    run_test gcc true
    run_test clang true
fi

{
    printf "END: "
    date
} >> runtimes.log
