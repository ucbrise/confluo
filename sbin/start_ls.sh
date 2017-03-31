#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/monolog-config.sh"
. "$MONOLOG_PREFIX/sbin/load-monolog-env.sh"

LOG_PATH=$sbin/../log

if [ "$CONCURRECY_CONTROL" = "" ]; then
  CONCURRENCY_CONTROL="read-stalled"
fi

if [ "$STORAGE" = "" ]; then
  STORAGE="in-memory"
fi

mkdir -p $LOG_PATH
$sbin/../build/libds/bin/lsserver --concurrency-control $CONCURRENCY_CONTROL\
  --storage $STORAGE 2>$LOG_PATH/ls.stderr 1>$LOG_PATH/ls.stdout &
