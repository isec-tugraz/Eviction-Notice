#!/bin/bash

MONITOR=./build-primitives/monitor-preadv2
# or use cachestat:
# MONITOR=./build-primitives/monitor-cachestat
MONITOR_FILE=$1

while true; do
    OUTPUT=$($MONITOR $MONITOR_FILE -1)
    LAST_LINE=$(echo "$OUTPUT" | tail -n 1)

    CACHE_VALUE=$(echo "$LAST_LINE" | grep -oP 'In cache: \K\d+')

    if [ "$CACHE_VALUE" -eq 0 ]; then
        break
    fi

    sleep 0.01
done

ps aux | grep '[e]vict-4-dynpressure' | awk '{print $2}' | xargs kill
ps aux | grep '[e]vict-3-baseline' | awk '{print $2}' | xargs kill
ps aux | grep '[e]vict-1-shlibs' | awk '{print $2}' | xargs kill
ps aux | grep '[e]vict-2-randaccess' | awk '{print $2}' | xargs kill

echo "Killed all eviction programs..."