# Run a shell command on all servers.
#
# Environment Variables
#
#   DIALOG_SERVERS    File naming remote servers.
#     Default is ${DIALOG_CONF_DIR}/servers.
#   DIALOG_CONF_DIR  Alternate conf dir. Default is ${DIALOG_HOME}/conf.
#   DIALOG_HOST_SLEEP Seconds to sleep between spawning remote commands.
#   DIALOG_SSH_OPTS Options passed to ssh when running remote commands.
##

usage="Usage: servers.sh [--config <conf-dir>] command..."

# if no args specified, show usage
if [ $# -le 0 ]; then
  echo $usage
  exit 1
fi

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/dialog-config.sh"

# If the servers file is specified in the command line,
# then it takes precedence over the definition in
# dialog-env.sh. Save it here.
if [ -f "$DIALOG_SERVERS" ]; then
  SERVERLIST=`cat "$DIALOG_SERVERS"`
fi

# Check if --config is passed as an argument. It is an optional parameter.
# Exit if the argument is not a directory.
if [ "$1" == "--config" ]
then
  shift
  conf_dir="$1"
  if [ ! -d "$conf_dir" ]
  then
    echo "ERROR : $conf_dir is not a directory"
    echo $usage
    exit 1
  else
    export DIALOG_CONF_DIR="$conf_dir"
  fi
  shift
fi

. "$DIALOG_PREFIX/sbin/load-dialog-env.sh"

if [ "$SERVERLIST" = "" ]; then
  if [ "$DIALOG_SERVERS" = "" ]; then
    if [ -f "${DIALOG_CONF_DIR}/hosts" ]; then
      SERVERLIST=`cat "${DIALOG_CONF_DIR}/hosts"`
    else
      SERVERLIST=localhost
    fi
  else
    SERVERLIST=`cat "${DIALOG_SERVERS}"`
  fi
fi



# By default disable strict host key checking
if [ "$DIALOG_SSH_OPTS" = "" ]; then
  DIALOG_SSH_OPTS="-o StrictHostKeyChecking=no"
fi

for host in `echo "$SERVERLIST"|sed  "s/#.*$//;/^$/d"`; do
  if [ -n "${DIALOG_SSH_FOREGROUND}" ]; then
    ssh $DIALOG_SSH_OPTS "$host" $"${@// /\\ }" \
      2>&1 | sed "s/^/$host: /"
  else
    ssh $DIALOG_SSH_OPTS "$host" $"${@// /\\ }" \
      2>&1 | sed "s/^/$host: /" &
  fi
  if [ "$DIALOG_HOST_SLEEP" != "" ]; then
    sleep $DIALOG_HOST_SLEEP
  fi
done

wait
