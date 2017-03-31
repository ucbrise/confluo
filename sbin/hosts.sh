# Run a shell command on all servers.
#
# Environment Variables
#
#   MONOLOG_SERVERS    File naming remote servers.
#     Default is ${MONOLOG_CONF_DIR}/servers.
#   MONOLOG_CONF_DIR  Alternate conf dir. Default is ${MONOLOG_HOME}/conf.
#   MONOLOG_HOST_SLEEP Seconds to sleep between spawning remote commands.
#   MONOLOG_SSH_OPTS Options passed to ssh when running remote commands.
##

usage="Usage: servers.sh [--config <conf-dir>] command..."

# if no args specified, show usage
if [ $# -le 0 ]; then
  echo $usage
  exit 1
fi

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/monolog-config.sh"

# If the servers file is specified in the command line,
# then it takes precedence over the definition in
# monolog-env.sh. Save it here.
if [ -f "$MONOLOG_SERVERS" ]; then
  SERVERLIST=`cat "$MONOLOG_SERVERS"`
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
    export MONOLOG_CONF_DIR="$conf_dir"
  fi
  shift
fi

. "$MONOLOG_PREFIX/sbin/load-monolog-env.sh"

if [ "$SERVERLIST" = "" ]; then
  if [ "$MONOLOG_SERVERS" = "" ]; then
    if [ -f "${MONOLOG_CONF_DIR}/hosts" ]; then
      SERVERLIST=`cat "${MONOLOG_CONF_DIR}/hosts"`
    else
      SERVERLIST=localhost
    fi
  else
    SERVERLIST=`cat "${MONOLOG_SERVERS}"`
  fi
fi



# By default disable strict host key checking
if [ "$MONOLOG_SSH_OPTS" = "" ]; then
  MONOLOG_SSH_OPTS="-o StrictHostKeyChecking=no"
fi

for host in `echo "$SERVERLIST"|sed  "s/#.*$//;/^$/d"`; do
  if [ -n "${MONOLOG_SSH_FOREGROUND}" ]; then
    ssh $MONOLOG_SSH_OPTS "$host" $"${@// /\\ }" \
      2>&1 | sed "s/^/$host: /"
  else
    ssh $MONOLOG_SSH_OPTS "$host" $"${@// /\\ }" \
      2>&1 | sed "s/^/$host: /" &
  fi
  if [ "$MONOLOG_HOST_SLEEP" != "" ]; then
    sleep $MONOLOG_HOST_SLEEP
  fi
done

wait
