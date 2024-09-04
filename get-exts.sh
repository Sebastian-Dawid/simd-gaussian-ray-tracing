#!/usr/bin/env sh
gcc -march=native -dM -E - < /dev/null | grep -E "SSE|AVX" | sort > extensions.h
