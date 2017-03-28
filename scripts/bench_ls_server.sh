#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

$sbin/../sbin/hosts.sh $sbin/../build/libds/bin/lssperf $@
