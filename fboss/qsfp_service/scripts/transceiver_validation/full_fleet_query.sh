#!/bin/bash
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

set -e  # Exit immediately if any command exits with non-zero status

# Configurable parameters and variables.
role=$1
platform=$2
process_counter=0
concurrent_process_limit=50
command_execution_time=60
pids=()


# Retrieves multiple command dumps for a given switch hostname (e.g. fsw001.p001.f01.ash8.tfbnw.net)
# and writes them to file.
get_tcvr_info() {
  host=$1
  platform=$2
  box_divider="BOX_DIVIDER"
  cmd_divider="CMD_DIVIDER"

  echo "SELECTED HOST: $host."
  eval "echo '$box_divider' >> ~/tcvr_configs/$platform/$host.out"
  eval "sush2 -n netops@$host sudo wedge_qsfp_util --reason 'dev' >> ~/tcvr_configs/$platform/$host.out 2> /dev/null"
  eval "echo '$cmd_divider' >> ~/tcvr_configs/$platform/$host.out"
  eval "fboss2 -H $host show transceiver >> ~/tcvr_configs/$platform/$host.out"
  eval "echo '$cmd_divider' >> ~/tcvr_configs/$platform/$host.out"
  eval "fboss2 -H $host show port >> ~/tcvr_configs/$platform/$host.out"
}
export -f get_tcvr_info


# Executed Code
echo "Querying all $platform hosts."
eval "mkdir -p ~/tcvr_configs/$platform/"
while read -r host; do
  eval "truncate -s 0 ~/tcvr_configs/$platform/$host.out"
  (( process_counter += 1 ))
  if [ $((process_counter % concurrent_process_limit)) -eq 0 ]; then
    echo "Waiting for lingering processes before spawning new processes."
    wait "${pids[@]}"
    pids=()
  fi
  timeout "$command_execution_time" bash -c "get_tcvr_info $host $platform" &
  pids+=($!)
done < <(eval "ods resolve 'smc(fboss.agent.$role.$platform, recurse=.*)'")
wait
