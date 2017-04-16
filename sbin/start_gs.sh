#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

SERVER_ID=${1:-"0"}
DATA_PATH=${2:-"$sbin/../data"}
mkdir -p $DATA_PATH

NODE_FILE=${3:-"$DATA_PATH/nodes.$SERVER_ID"}
LINK_FILE=${4:-"$DATA_PATH/links.$SERVER_ID"}
HOST_LIST=${5:-"$sbin/../conf/hosts"}

echo "SERVER_ID=$SERVER_ID, NODE_FILE=$NODE_FILE, LINK_FILE=$LINK_FILE"
echo "HOST_LIST=$HOST_LIST"


LOG_PATH=$sbin/../log
mkdir -p $LOG_PATH
$sbin/../build/app/graphstore/libgs/bin/gsserver --server-id $SERVER_ID\
  --load-nodes $NODE_FILE --load-links $LINK_FILE --host-list $HOST_LIST\
  2>$LOG_PATH/gs.stderr 1>$LOG_PATH/gs.stdout &
