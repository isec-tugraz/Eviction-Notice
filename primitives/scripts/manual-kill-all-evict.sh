#!/bin/sh

ps aux | grep '[e]vict-4-dynpressure' | awk '{print $2}' | xargs kill
ps aux | grep '[e]vict-3-baseline' | awk '{print $2}' | xargs kill
ps aux | grep '[e]vict-1-shlibs' | awk '{print $2}' | xargs kill
ps aux | grep '[e]vict-2-randaccess' | awk '{print $2}' | xargs kill