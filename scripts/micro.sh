for i in 1 2 4 8 16 32 64; do
  for mix in "0.0-0.0-1.0-0.0"; do
    ./build/bench/bin/mbench -b throughput-${mix} -n $i ~/tpch.ser
  done
done
