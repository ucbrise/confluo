#!/bin/bash
set -e

mkdir -p build
cd build
cmake ..

START=$(date +%s)
make
make test ARGS="-VV"
END=$(date +%s)
echo "Total build time (real) = $(( $END - $START )) seconds"
