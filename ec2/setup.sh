#!/usr/bin/env bash

curdir="`dirname "$0"`"
curdir="`cd "$curdir"; pwd`"

sudo yum -y update
sudo yum -y groupinstall "Development Tools"
sudo yum -y install gcc-c++ cmake boost-devel java-1.7.0-openjdk* ant

cd $curdir/..
mkdir build
cd build
cmake -DBUILD_TESTS=OFF -DWITH_JAVA_CLIENT=OFF -DWITH_PY_CLIENT=OFF ..
make -j

cd ..
./sbin/sync .
