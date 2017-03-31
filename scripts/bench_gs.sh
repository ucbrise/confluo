#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

ops=(
add_node
update_node
delete_node
get_node
add_link
update_link
delete_link
get_link
get_link_list
count_links
)

for tail_scheme in "read-stalled" "write-stalled"; do
  for op in ${ops[@]}; do
    for num_threads in 1 2 4 8 16 32 64 128; do
      $sbin/../build/app/graphstore/libgs/bin/gsperf --num-threads=$num_threads\
        --bench-type=$op --tail-scheme=$tail_scheme --output-dir=$tail_scheme
    done
  done
done
