#!/bin/bash
# partx wrapper: uses kpartx as fallback when the kernel can't create
# partition device nodes natively.
#
# In Sandcastle containers, /dev is a tmpfs (not the host's devtmpfs).
# When partx tells the kernel to create partition devices, they land on the
# real host devtmpfs and are invisible to the container. kpartx sidesteps
# this by using device-mapper + mknod in userspace, creating /dev/mapper/*
# entries directly on the container's visible /dev.
#
# Usage: install as /usr/sbin/partx after renaming the real partx to
#        /usr/sbin/partx.real.

LOG=/tmp/partx_wrapper.log
echo "=== partx_wrapper called: $* ===" >> "$LOG"

REAL_PARTX=$(which partx.real 2>/dev/null || echo /usr/sbin/partx.real)

# Find the device argument and operation type
device=
op=
for arg in "$@"; do
    case "$arg" in
        /dev/*) device=$arg ;;
        --add|-a) op=add ;;
        --delete|-d) op=delete ;;
    esac
done

[ -z "$device" ] && { echo "no device found in args" >> $LOG; exit 1; }

# For non-add/delete operations (--show, --list, etc.), pass through directly
if [ "$op" != "add" ] && [ "$op" != "delete" ]; then
    echo "passthrough: forwarding to real partx" >> $LOG
    "$REAL_PARTX" "$@" 2>>$LOG
    exit $?
fi

# For --delete calls, remove kpartx mappings and symlinks
if [ "$op" = "delete" ]; then
    echo "delete: removing kpartx mappings for $device" >> $LOG
    kpartx -d "$device" 2>>$LOG || true
    loopname=$(basename "$device")
    rm -f /dev/"${loopname}"p* 2>/dev/null
    "$REAL_PARTX" "$@" 2>>$LOG || true
    exit 0
fi

# For --add calls: try real partx first
loopname=$(basename "$device")
"$REAL_PARTX" "$@" 2>>$LOG || true
if ls /dev/"${loopname}"p* >/dev/null 2>&1; then
    echo "real partx created partition devices" >> $LOG
    exit 0
fi

# Fallback: use kpartx to create device-mapper entries
echo "using kpartx fallback for $device" >> $LOG
kpartx -av "$device" >> $LOG 2>&1

# Symlink /dev/mapper/loopNpM -> /dev/loopNpM where KIWI expects them
for dm in /dev/mapper/"${loopname}"p*; do
    [ -e "$dm" ] || continue
    pnum="${dm##*p}"
    ln -sf "$dm" "/dev/${loopname}p${pnum}"
    echo "  CREATED: /dev/${loopname}p${pnum} -> $dm" >> $LOG
done
exit 0
