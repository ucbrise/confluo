for dataset in "tpch" "conviva"; do
  for bench in "latency-get" "latency-search" "latency-append" "latency-delete"; do
    ../build/bench/bin/mbench -b ${bench} ~/${dataset}.ser
    mkdir -p $dataset/micro
    mv latency_* $dataset/micro/
  done
done


for i in 1 2 4 8 16 32 64; do
  for mix in "0.0-0.0-1.0-0.0" "0.34-0.33-0.33-0.0" "0.5-0.5-0.0-0.0"; do
    for dataset in "tpch" "conviva"; do
    	../build/bench/bin/mbench -b throughput-${mix} -n $i ~/${dataset}.ser
	mkdir -p $dataset/micro
        mv throughput_* $dataset/micro/
    done
  done
done
