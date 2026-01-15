#!/bin/sh

la_shlibs=$(cat /proc/*/maps 2>/dev/null | grep "\.so\." | awk '{print $6}' | sort | uniq | xargs)

echo $la_shlibs

# If you need snaps :| (horrendous thing)
# find /snap/ -name "*.so" | sort | uniq | xargs