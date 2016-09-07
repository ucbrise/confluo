workload=$1
dataset=$2

if [ "$workload" = "read" ]; then
  mix="0.50_0.50_0.00_0.00"
elif [ "$workload" = "write" ]; then
  mix="0.00_0.00_1.00_0.00"
elif [ "$workload" = "mix" ]; then
  mix="0.34_0.33_0.33_0.00"
else
  echo "Invalid workload $workload"
  exit -1
fi

for i in 1 2 4 8 16 32 64 128 256; do
  awk '{ sum += $1 } END { print sum }' $dataset/macro/throughput_${mix}_$i
done
