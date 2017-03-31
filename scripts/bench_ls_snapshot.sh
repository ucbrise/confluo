#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

num_writers=${1:-"1"}

function cleanup() {
  echo "Stopping coordinator..."
  $sbin/../sbin/stop_coord.sh

  echo "Stopping all servers..."
  $sbin/../sbin/hosts.sh $sbin/../sbin/stop_ls.sh
  
  echo "Removing previous results..."
  $sbin/../sbin/hosts.sh rm -f $sbin/../results/throughput-append-1.txt
}

function setup() {
  # Cleanup
  cleanup
  
  # Start servers on all servers
  echo "Starting servers on all servers..."
  $sbin/../sbin/hosts.sh $sbin/../sbin/start_ls.sh
  echo "Waiting for servers to intialize..."
  sleep 5

  # Start coordinator on current node
  echo "Starting coordinator..."
  $sbin/../sbin/start_coord.sh
  echo "Waiting for coordinator to intialize..."
  sleep 2
}

function benchmark_update() {
  # Start update benchmark on all servers
  num_writers=$1
  echo "Starting benchmark..."
  for i in `seq 1 $num_writers`; do
    $sbin/../sbin/hosts.sh $sbin/bench_ls_server.sh --bench-op append --output-dir $sbin/../results &
  done
  wait
}

setup

benchmark_update $num_writers

$sbin/../sbin/hosts.sh cat $sbin/../results/throughput-append-1.txt > $sbin/../results/throughput-append.txt

echo "Benchmark finished; cleaning up..."
cleanup
