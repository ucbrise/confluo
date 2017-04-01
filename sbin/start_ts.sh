#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

LOG_PATH=$sbin/../log

mkdir -p $LOG_PATH
$sbin/../build/app/timeseries/libtimeseries/bin/tsserver 2>$LOG_PATH/ts.stderr 1>$LOG_PATH/ts.stdout &
