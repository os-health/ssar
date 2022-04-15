#!/bin/bash

LOCKFILE="/run/lock/os_health/sresar.pid"
exec {FD}<> $LOCKFILE
if [ ! -e /etc/ssar/sresar.disable ];then
    if flock -x -n $FD; then
        flock --unlock $FD
        /usr/local/bin/sresar -D > /dev/null 2>&1 & 
    fi
fi
