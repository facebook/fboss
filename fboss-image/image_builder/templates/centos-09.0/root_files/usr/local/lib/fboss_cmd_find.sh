#!/bin/bash
# Find and run the given FBOSS command.
# Usage: fboss_cmd_find.sh <binary_name> [args...]
set -e

if [ -z "$1" ]; then
  echo "No command specified" >&2
  exit 1
fi

cmd="$1"
shift

case "$cmd" in
fboss2 | fboss2-dev | diag_shell_client)
  # Forwarding stack commands
  update_prefix="fboss-forwarding"
  ;;

fw_util | sensor_service_client | showtech | weutil)
  # Platform stack commands
  update_prefix="fboss-platform_stack"
  ;;

*)
  echo "Unknown FBOSS command: $cmd" >&2
  exit 1
  ;;
esac

update_path=$(ls -1 /updates/${update_prefix}-*/opt/fboss/bin/${cmd} | head -n 1)
if [ -x "$update_path" ]; then
  echo exec "$update_path" "$@"
else
  exec "/opt/fboss/bin/${cmd}" "$@"
fi
echo "Failed to find fboss command"
exit 1
