#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/dialog-config.sh"
. "$DIALOG_PREFIX/sbin/load-dialog-env.sh"

LOG_PATH="$sbin/../log"
mkdir -p $LOG_PATH

if [ "$HOST_ENDPOINT" = "" ]; then
  HOST_ENDPOINT="0.0.0.0:9090"
fi

if [ "$DATA_PATH" = "" ]; then
  DATA_PATH="$sbin/../data"
  mkdir -p $DATA_PATH
fi

mkdir -p $LOG_PATH
$sbin/../build/bin/dialogd --endpoint=$HOST_ENDPOINT --data-path $DATA_PATH $@\
   2>$LOG_PATH/dialog.stderr 1>$LOG_PATH/dialog.stdout &
