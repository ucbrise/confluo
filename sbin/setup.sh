#!/bin/bash


CONFLUO_DISTRIBUTION_HOME=/home/confluo/
CONFLUO_SRC_HOME=/home/workspace/confluo
echo "source home:" $CONFLUO_SRC_HOME
echo "distribution home: "$CONFLUO_DISTRIBUTION_HOME

if [ ! -d $CONFLUO_DISTRIBUTION_HOME ];then
    mkdir $CONFLUO_DISTRIBUTION_HOME
else
    echo $CONFLUO_DISTRIBUTION_HOME" exist"
fi

cd $CONFLUO_DISTRIBUTION_HOME
if [ ! -d ./build ];then
    mkdir build
else
    echo $CONFLUO_DISTRIBUTION_HOME" exist"
fi
cp -r $CONFLUO_SRC_HOME/build/librpc/confluod  $CONFLUO_SRC_HOME/build/bin
cp -r $CONFLUO_SRC_HOME/build/bin ./build/
cp -r $CONFLUO_SRC_HOME/conf  ./
cp -r  $CONFLUO_SRC_HOME/sbin  ./
#rm  ./build/bin/stest ./build/bin/tstest