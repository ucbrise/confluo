#!/bin/bash

CONFLUO_DISTRIBUTION_HOME=/export/confluo/
CONFLUO_SRC_HOME="/export/workspace/confluo"
echo $CONFLUO_SRC_HOME
echo $CONFLUO_DISTRIBUTION_HOME
mkdir $CONFLUO_DISTRIBUTION_HOME
cd $CONFLUO_DISTRIBUTION_HOME

mkdir build
cp -r $CONFLUO_SRC_HOME/build/bin ./build/
cp -r $CONFLUO_SRC_HOME/conf  ./
cp -r  $CONFLUO_SRC_HOME/sbin  ./
rm  ./build/bin/stest ./build/bin/tstest