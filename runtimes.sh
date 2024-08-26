#!/usr/bin/env sh

touch runtimes.log

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
    if [ "$1" = "clang" ]; then
        clang="--with-clang"
    fi
    make build -j32 ARGS="${clang} ${svml}"
    for mode in $(seq 2 8); do
        printf "\tMODE %s ST: " "${mode}" >> runtimes.log
        ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj --frames 10 -t1 --tiles 16 -m "${mode}" >> runtimes.log

        if [ "${mode}" -gt 4 ]; then
            printf "\tMODE %s MT(32): " "${mode}" >> runtimes.log
            ./build/bin/release/volumetric-ray-tracer -q -f ./test-objects/cube.obj --frames 10 -t32 --tiles 16 -m "${mode}" >> runtimes.log
        fi
    done
}

run_test gcc false
run_test gcc true
run_test clang false
run_test clang true
