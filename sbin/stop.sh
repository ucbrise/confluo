PIDS=`pgrep mica_server`
for pid in $PIDS; do
  echo "Killing $pid..."
  kill -9 $pid
done
