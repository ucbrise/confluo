hostname=${1:-localhost}

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

ssh -o StrictHostKeyChecking=no $hostname "killall server"

echo "Benchmarking server at $hostname"
for dataset in "tpch" "conviva"; do
  for bench in "get-latency" "search-latency" "append-latency" "delete-latency"; do
    ssh -o StrictHostKeyChecking=no $hostname "nohup /home/ec2-user/log-store/build/server/bin/server >/dev/null 2>/dev/null &"
    sleep 5
    $sbin/../build/bench/bin/lsbench -b $bench -h $hostname ~/${dataset}
    mkdir -p $dataset/macro
    mv latency_* $dataset/macro/
    ssh -o StrictHostKeyChecking=no $hostname "killall server"
    sleep 1
  done
done
