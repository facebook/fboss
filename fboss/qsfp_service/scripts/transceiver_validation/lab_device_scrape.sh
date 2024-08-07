#!/bin/bash
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

platforms=(darwin)
command_execution_time=120

get_wedge_dump() {
  platform=$1
  host=$2
  echo "Querying for $host."
  eval "sush2 -n netops@$host 'sudo wedge_qsfp_util --direct-i2c' --reason 'dev' > ~/lab_configs/$platform/$host.out 2> /dev/null"
}
export -f get_wedge_dump

for platform in "${platforms[@]}"; do
  pids=()
  eval "mkdir -p ~/lab_configs/$platform/"
  echo "Querying all $platform lab devices."
  while read -r host; do
    timeout "$command_execution_time" bash -c "get_wedge_dump $platform $host" &
    pids+=($!)
  done < <(eval "python3 fetch_platform_lab_devices.py $platform")
  wait "${pids[@]}"
  echo "Completed querying all $platform lab devices."
done
