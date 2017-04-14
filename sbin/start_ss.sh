#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

LOG_PATH=$sbin/../log

if [ "$DATA_PATH" = "" ]; then
  DATA_PATH="."
fi

mkdir -p $LOG_PATH
$sbin/../build/app/streaming/libstream/bin/sserver --data-path $DATA_PATH 2>$LOG_PATH/ts.stderr 1>$LOG_PATH/ss.stdout &
