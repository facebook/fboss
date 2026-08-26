#!/bin/bash
# partx wrapper: creates partition device nodes when the kernel cannot.
#
# In Sandcastle containers /dev is a tmpfs, no udev runs, and the loop driver
# is loaded with max_part=0, so "partx --add" never produces /dev/loopNpM.
# Fall back through the remaining mechanisms, most-supported first:
#
#   1. kpartx, i.e. device-mapper plus mknod in userspace. Unavailable when
#      dm_mod is not loaded on the worker, where opening /dev/mapper/control
#      fails with EPERM.
#   2. one loop device per partition, bound to the backing file at the
#      partition offset. Needs only the loop driver.
#
# Fallback 2 hands KIWI a symlink to an independent loop device rather than a
# real partition of $device, so the two share no block device. Flush before
# detaching, and do not assume tools can walk from the node back to the disk.
#
# Usage: install as /usr/sbin/partx after renaming the real partx to
#        /usr/sbin/partx.real.

LOG=/tmp/partx_wrapper.log
STATE_DIR=/tmp/partx_wrapper.state

log() {
    echo "$*" >>"$LOG"
}

log "=== partx_wrapper called: $* ==="

REAL_PARTX=$(command -v partx.real 2>/dev/null || echo /usr/sbin/partx.real)

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

[ -z "$device" ] && { log "no device found in args"; exit 1; }

loopname=$(basename "$device")
state="${STATE_DIR}/${loopname}"

# Detach the per-partition loop devices recorded by fallback 2, if any.
detach_children() {
    [ -f "$state" ] || return 0
    while read -r child; do
        [ -n "$child" ] || continue
        blockdev --flushbufs "$child" 2>>"$LOG" || true
        losetup -d "$child" 2>>"$LOG" || true
        log "  detached $child"
    done <"$state"
    rm -f "$state"
}

# For non-add/delete operations (--show, --list, etc.), pass through directly
if [ "$op" != "add" ] && [ "$op" != "delete" ]; then
    log "passthrough: forwarding to real partx"
    "$REAL_PARTX" "$@" 2>>"$LOG"
    exit $?
fi

# For --delete calls, tear down every mapping this wrapper may have created
if [ "$op" = "delete" ]; then
    log "delete: removing mappings for $device"
    detach_children
    kpartx -d "$device" 2>>"$LOG" || true
    rm -f /dev/"${loopname}"p* 2>/dev/null
    "$REAL_PARTX" "$@" 2>>"$LOG" || true
    exit 0
fi

# For --add calls: try real partx first
"$REAL_PARTX" "$@" 2>>"$LOG" || true
if ls /dev/"${loopname}"p* >/dev/null 2>&1; then
    log "real partx created partition devices"
    exit 0
fi

# Fallback 1: use kpartx to create device-mapper entries, symlinked where
# KIWI expects them
log "using kpartx fallback for $device"
if kpartx -av "$device" >>"$LOG" 2>&1; then
    created=
    for dm in /dev/mapper/"${loopname}"p*; do
        [ -e "$dm" ] || continue
        pnum="${dm##*p}"
        ln -sf "$dm" "/dev/${loopname}p${pnum}"
        log "  CREATED: /dev/${loopname}p${pnum} -> $dm"
        created=yes
    done
    [ -n "$created" ] && exit 0
fi

# Fallback 2: bind one loop device per partition. partx reads the partition
# table with libblkid, so it still reports the layout the kernel refused to
# instantiate.
log "using loop-offset fallback for $device"

backing=$(losetup -nO BACK-FILE "$device" 2>>"$LOG")
if [ -z "$backing" ]; then
    log "  ERROR: no backing file found for $device"
    exit 1
fi

sector_size=$(blockdev --getss "$device" 2>/dev/null || echo 512)

mkdir -p "$STATE_DIR"
: >"$state"

"$REAL_PARTX" -g -o NR,START,SECTORS -r "$device" 2>>"$LOG" |
    while read -r pnum start sectors; do
        [ -n "$pnum" ] || continue
        offset=$((start * sector_size))
        size=$((sectors * sector_size))
        child=$(losetup -f --show -o "$offset" --sizelimit "$size" \
            "$backing" 2>>"$LOG")
        if [ -z "$child" ]; then
            log "  ERROR: losetup failed for partition ${pnum}"
            continue
        fi
        echo "$child" >>"$state"
        ln -sf "$child" "/dev/${loopname}p${pnum}"
        log "  CREATED: /dev/${loopname}p${pnum} -> $child (offset ${offset}, size ${size})"
    done

if ! ls /dev/"${loopname}"p* >/dev/null 2>&1; then
    log "  ERROR: no partition devices could be created for $device"
    exit 1
fi
exit 0
