#!/bin/sh

BASEDIR=`dirname $0`/..
BASEDIR=`(cd "$BASEDIR"; pwd)`

# If a specific java binary isn't specified search for the standard 'java' binary
if [ -z "$JAVACMD" ] ; then
  if [ -n "$JAVA_HOME"  ] ; then
    if [ -x "$JAVA_HOME/jre/sh/java" ] ; then
      # IBM's JDK on AIX uses strange locations for the executables
      JAVACMD="$JAVA_HOME/jre/sh/java"
    else
      JAVACMD="$JAVA_HOME/bin/java"
    fi
  else
    JAVACMD=`which java`
  fi
fi

CLASSPATH="$BASEDIR"/conf/:"$BASEDIR"/lib/*
LOGDIR="$BASEDIR/log/"
JAVA_OPTS="-Xdebug "
echo "$CLASSPATH"

if [ ! -x "$JAVACMD" ] ; then
  echo "Error: JAVA_HOME is not defined correctly."
  echo "  We cannot execute $JAVACMD"
  exit 1
fi

if [ -z "$OPTS_MEMORY" ] ; then
    OPTS_MEMORY="-Xms1G -Xmx2G -server -XX:MaxPermSize=256M -Xss256K"
fi

 "$JAVACMD" $JAVA_OPTS \
  $OPTS_MEMORY \
  -classpath "$CLASSPATH" \
  -Dactivemq_log="${LOGDIR}" \
  -Dbasedir="$BASEDIR" \
  -Dfile.encoding="UTF-8" \
  confluo.streaming.ProducerBenchmark \
#  "$@" >/dev/null 2>/dev/null &
