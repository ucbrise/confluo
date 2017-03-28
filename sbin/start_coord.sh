#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/monolog-config.sh"

LOG_PATH=$sbin/../log

if [ "$HOST_LIST" = "" ]; then
  HOST_LIST="$MONOLOG_CONF_DIR/hosts"
fi

if [ "$SLEEP_US" = "" ]; then
  SLEEP_US="0"
fi

mkdir -p $LOG_PATH
$sbin/../build/libds/bin/lscoordinator --host-list $HOST_LIST\
  --sleep-us $SLEEP_US 2>$LOG_PATH/coord.stderr 1>$LOG_PATH/coord.stdout &
