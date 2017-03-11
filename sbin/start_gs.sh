#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

LOG_PATH=$sbin/../log

mkdir -p $LOG_PATH
$sbin/../build/app/graphstore/libgs/bin/gsserver 2>$LOG_PATH/gs.stderr 1>$LOG_PATH/gs.stdout &
