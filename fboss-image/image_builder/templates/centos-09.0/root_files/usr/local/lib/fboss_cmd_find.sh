#!/bin/bash
# Find and run the given FBOSS command.
# Usage: fboss_cmd_find.sh <binary_name> [args...]
set -e

# Install locations during image build
default_forwarding_stack_path="/opt/fboss/bin"
default_platform_stack_path="/opt/fboss/bin"

if [ -z "$1" ]; then
  echo "No command specified" >&2
  exit 1
fi

cmd="$1"
shift

case "$cmd" in
fboss2 | fboss2-dev | diag_shell_client)
  # Forwarding stack commands
  stack_path=${default_forwarding_stack_path}
  ;;

fw_util | sensor_service_client | showtech | weutil)
  # Platform stack commands
  stack_path=${default_platform_stack_path}
  ;;

*)
  echo "Unknown FBOSS command: $cmd" >&2
  exit 1
  ;;
esac

exec "${stack_path}/${cmd}" "$@"
