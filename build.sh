#!/bin/bash

mkdir -p build
cd build
if [[ "$OSTYPE" == "darwin"* ]]; then
	echo "Operating System is Mac OS X"
	cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/include ../
else
	cmake ../
fi
START=$(date +%s)
make
END=$(date +%s)
echo "Total Build time (real) = $(( $END - $START )) seconds"
