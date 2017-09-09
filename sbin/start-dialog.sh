#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/dialog-config.sh"
. "$DIALOG_PREFIX/sbin/load-dialog-env.sh"

LOG_PATH="$sbin/../log"
mkdir -p $LOG_PATH

if [ "$BIND_ADDRESS" = "" ]; then
  BIND_ADDRESS="0.0.0.0"
fi

if [ "$BIND_PORT" = "" ]; then
  BIND_PORT="9090"
fi

if [ "$DATA_PATH" = "" ]; then
  DATA_PATH="$sbin/../data"
  mkdir -p $DATA_PATH
fi

mkdir -p $LOG_PATH
$sbin/../build/bin/dialogd --address=$BIND_ADDRESS --port $BIND_PORT \
  --data-path $DATA_PATH 2>$LOG_PATH/dialog.stderr 1>$LOG_PATH/dialog.stdout &
