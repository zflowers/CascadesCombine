#!/bin/bash
# Usage: ./track_memory.sh <command> [args...]
# Example:
# ./track_memory.sh combineTool.py -M AsymptoticLimits -d mydir/*.txt --there --parallel 4

LOG="memtrack_$(date +%Y%m%d_%H%M%S).log"
echo "[INFO] Logging memory usage to $LOG"

# Run the target command in background
"$@" &
PID=$!

# Start the memory monitor
(
  echo "Time PID %MEM RSS(KB) COMMAND" >> "$LOG"
  while ps -p "$PID" > /dev/null 2>&1; do
    date +"%T" >> "$LOG"
    ps -o pid,%mem,rss,cmd -p "$PID" >> "$LOG"
    sleep 5
  done
) &

MONITOR_PID=$!

# Wait for the main command to finish
wait $PID
EXIT_CODE=$?

# Clean up monitor
kill $MONITOR_PID 2>/dev/null

echo "[INFO] Process $PID finished with exit code $EXIT_CODE"
echo "[INFO] Memory log saved at: $LOG"
exit $EXIT_CODE

