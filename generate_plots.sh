#!/usr/bin/env sh
make build -j32
./build/bin/release/accuracy-test
./build/bin/release/cycles-test
./build/bin/release/timing-test
./build/bin/release/transmittance-test
