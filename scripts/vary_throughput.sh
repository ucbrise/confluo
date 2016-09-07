hostname=${1:-localhost}
dataset=${2:-tpch}

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

ssh -o StrictHostKeyChecking=no $hostname "killall server"

delete=0.00

echo "Benchmarking server at $hostname for dataset $dataset"
for search in 0.00 0.10 0.20 0.30 0.40 0.50 0.60 0.70 0.80 0.90 1.00; do
  remaining=`echo "scale=2;1.00-$search" | bc`
  for type in get insert mix; do
    if [ "$type" = "get" ]; then
      get=`printf "%0.2f" $remaining`
      insert=0.00
    elif [ "$type" = "insert" ]; then
      get=0.00
      insert=`printf "%0.2f" $remaining`
    elif [ "$type" = "mix" ]; then
      half=`echo "scale=2;$remaining*0.50" | bc`
      get=`printf "%0.2f" $half`
      insert=`printf "%0.2f" $half`
    fi
    mix=${get}-${search}-${insert}-${delete}
    for i in 2 4 8 16 32 64 128 256; do
      ssh -o StrictHostKeyChecking=no $hostname "nohup /home/ec2-user/log-store/build/server/bin/server >/dev/null 2>/home/ec2-user/throughput-${mix}-${i}.log &"
      sleep 5
      bench_type="throughput-${mix}"
      echo "Bench-Type = $bench_type"
      $sbin/../build/bench/bin/lsbench -b throughput-${mix} -n $i -h $hostname ~/$dataset
      mkdir -p $dataset/vary
      mv throughput_* $dataset/vary/
      ssh -o StrictHostKeyChecking=no $hostname "killall server"
      sleep 5
    done
  done
done
