#!/usr/bin/env bash

reverse() {
  sed '1!G;h;$!d' $1
}

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/dialog-config.sh"
. "$DIALOG_PREFIX/sbin/load-dialog-env.sh"

SERVERLIST=`reverse ${DIALOG_CONF_DIR}/hosts`

# By default disable strict host key checking
if [ "$DIALOG_SSH_OPTS" = "" ]; then
  DIALOG_SSH_OPTS="-o StrictHostKeyChecking=no"
fi

if [ "$DIALOG_PATH" == "" ]; then
  DIALOG_PATH="$HOME/dialog"
fi

if [ "$DIALOG_PORT" == "" ]; then
  DIALOG_PORT="9090"
fi

HOSTS=`echo "$SERVERLIST"|sed  "s/#.*$//;/^$/d"`
SAVEIFS=$IFS
IFS=$'\n'
HOSTS=($HOSTS)
IFS=$SAVEIFS

TAIL=${HOSTS[0]}
echo "Setting up tail=$TAIL"
ssh $DIALOG_SSH_OPTS $TAIL $DIALOG_PATH/sbin/start-dialog.sh \
  --endpoint "0.0.0.0:$DIALOG_PORT" \
  2>&1 | sed "s/^/$host: /"
for (( i=1; i<${#HOSTS[@]}; i++ )); do
  HOST=${HOSTS[$i]}
  SUCCESSOR=${HOSTS[$i-1]}
  echo "Setting up host=$HOST with successor=$SUCCESSOR, tail=$TAIL"
  ssh $DIALOG_SSH_OPTS $HOST $DIALOG_PATH/sbin/start-dialog.sh \
    --endpoint "0.0.0.0:$DIALOG_PORT" \
    --successor "$SUCCESSOR:$DIALOG_PORT" \
    --tail "$TAIL:$DIALOG_PORT" \
    2>&1 | sed "s/^/$HOST: /"
done
