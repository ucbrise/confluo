#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

res_dir=${1:-"results"}

# Start servers on all servers
$sbin/../sbin/hosts.sh $sbin/../sbin/start_ls.sh
sleep 5

# Start coordinator on current node
$sbin/../sbin/start_coord.sh
sleep 2

# Start update benchmark on all servers
$sbin/bench_ls_server.sh --bench-op append --output-dir $sbin/../results
