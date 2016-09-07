function greaterthan() {
  num1=$1
  num2=$2
  echo $num1'>'$num2 | bc -l
}

delete=0.00
path=$1
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
    mix=${get}_${search}_${insert}_${delete}
    max=0.00
    for i in 2 4 8 16 32 64 128; do
      thput=`awk '{ sum += $1 } END { print sum }' ${path}/throughput_${mix}_${i}`
      gt=`greaterthan $thput $max`
      if [ "$gt" = "1"  ]; then
        max=$thput
      fi
    done
    echo "${search} ${get} ${insert} ${max}" >> ${path}_${type}.agg
  done
done
