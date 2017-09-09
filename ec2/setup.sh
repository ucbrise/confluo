#!/usr/bin/env bash

curdir="`dirname "$0"`"
curdir="`cd "$curdir"; pwd`"

sudo yum -y update
sudo yum -y groupinstall "Development Tools"
sudo yum -y install gcc-c++ boost-devel java-1.7.0-openjdk* ant

wget https://cmake.org/files/v3.9/cmake-3.9.2.tar.gz
tar xvzf cmake-3.9.2.tar.gz
cd cmake-3.9.2
./bootstrap
make
make install

cd $curdir/..
mkdir build
cd build
cmake -DBUILD_TESTS=OFF -DWITH_JAVA_CLIENT=OFF -DWITH_PY_CLIENT=OFF ..
make -j

cd ..
./sbin/sync .
