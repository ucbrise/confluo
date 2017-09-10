#!/usr/bin/env bash

reverse() {
  sed '1!G;h;$!d' $1
}

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/dialog-config.sh"
. "$DIALOG_PREFIX/sbin/load-dialog-env.sh"

SERVERLIST=`reverse ${DIALOG_CONF_DIR}/hosts`

if [ "$DIALOG_PATH" == "" ]; then
  DIALOG_PATH="$HOME/dialog"
fi

HOSTS=`echo "$SERVERLIST"|sed  "s/#.*$//;/^$/d"`
SAVEIFS=$IFS
IFS=$'\n'
HOSTS=($HOSTS)
IFS=$SAVEIFS

for (( i=0; i<${#HOSTS[@]}; i++ )); do
  HOST=${HOSTS[$i]}
  echo "Taking down ${HOSTS[$i]}"
  ssh $DIALOG_SSH_OPTS $HOST $DIALOG_PATH/sbin/stop-dialog.sh \
    2>&1 | sed "s/^/$HOST: /"
done
