#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

res_dir=${1:-"results"}
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
for op in ${ops[@]}; do
  for num_threads in 1 2 4 8 16 32; do
    $sbin/../build/app/graphstore/libgs/bin/gsperf --num-threads=$num_threads\
      --bench-type=$op --output-dir=$res_dir
  done
done
