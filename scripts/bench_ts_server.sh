#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

$sbin/../build/app/timeseries/libtimeseries/bin/tssperf $@
