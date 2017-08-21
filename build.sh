#!/bin/bash
set -e

mkdir -p build
cd build
if [[ "$OSTYPE" == "darwin"* ]]; then
  cmake .. -DCMAKE_CXX_COMPILER=g++
else
  cmake ..
fi

START=$(date +%s)
make
make test ARGS="-VV"
END=$(date +%s)
echo "Total build time (real) = $(( $END - $START )) seconds"
