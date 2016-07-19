for i in 1 2 4 8 16 32 64; do
  for mix in "0.0-0.0-1.0-0.0"; do #"0.5-0.5-0.0-0.0" "0.34-0.33-0.33-0.0"; do
    ./build/server/bin/server &
    sleep 5
    ./build/bench/bin/lsbench -b throughput-${mix} -n $i ~/tpch
    killall server
  done
done
