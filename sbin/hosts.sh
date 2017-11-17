# Run a shell command on all servers.
#
# Environment Variables
#
#   CONFLUO_SERVERS    File naming remote servers.
#     Default is ${CONFLUO_CONF_DIR}/servers.
#   CONFLUO_CONF_DIR  Alternate conf dir. Default is ${CONFLUO_HOME}/conf.
#   CONFLUO_HOST_SLEEP Seconds to sleep between spawning remote commands.
#   CONFLUO_SSH_OPTS Options passed to ssh when running remote commands.
##

usage="Usage: servers.sh [--config <conf-dir>] command..."

# if no args specified, show usage
if [ $# -le 0 ]; then
  echo $usage
  exit 1
fi

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/confluo-config.sh"

# If the servers file is specified in the command line,
# then it takes precedence over the definition in
# confluo-env.sh. Save it here.
if [ -f "$CONFLUO_SERVERS" ]; then
  SERVERLIST=`cat "$CONFLUO_SERVERS"`
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
    export CONFLUO_CONF_DIR="$conf_dir"
  fi
  shift
fi

. "$CONFLUO_PREFIX/sbin/load-confluo-env.sh"

if [ "$SERVERLIST" = "" ]; then
  if [ "$CONFLUO_SERVERS" = "" ]; then
    if [ -f "${CONFLUO_CONF_DIR}/hosts" ]; then
      SERVERLIST=`cat "${CONFLUO_CONF_DIR}/hosts"`
    else
      SERVERLIST=localhost
    fi
  else
    SERVERLIST=`cat "${CONFLUO_SERVERS}"`
  fi
fi



# By default disable strict host key checking
if [ "$CONFLUO_SSH_OPTS" = "" ]; then
  CONFLUO_SSH_OPTS="-o StrictHostKeyChecking=no"
fi

for host in `echo "$SERVERLIST"|sed  "s/#.*$//;/^$/d"`; do
  if [ -n "${CONFLUO_SSH_FOREGROUND}" ]; then
    ssh $CONFLUO_SSH_OPTS "$host" $"${@// /\\ }" \
      2>&1 | sed "s/^/$host: /"
  else
    ssh $CONFLUO_SSH_OPTS "$host" $"${@// /\\ }" \
      2>&1 | sed "s/^/$host: /" &
  fi
  if [ "$CONFLUO_HOST_SLEEP" != "" ]; then
    sleep $CONFLUO_HOST_SLEEP
  fi
done

wait
