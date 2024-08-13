#!/bin/bash
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

platforms=(darwin elbert fuji wedge100s wedge400 wedge400c yamp)
command_execution_time=60

get_wedge_dump() {
  platform=$1
  host_and_config=$2

  IFS=',' read -ra fields <<< "$host_and_config"
  host="${fields[0]}"
  hw_config="${fields[1]}"

  echo "Querying for $host."

  if [[ $hw_config == "" ]]
  then
    echo -e "$platform\n" > "$HOME/lab_configs/$platform/$host.out"
  else
    echo -e "${platform}_${hw_config}\n" > "$HOME/lab_configs/$platform/$host.out"
  fi

  if [[ $platform == "fuji" && $hw_config == "5x16Q" ]]
  then
    echo "$host is a 5pim fuji device"
    eval "sush2 -n netops@$host 'sudo wedge_qsfp_util --direct-i2c --force-5pim-fuji' --reason 'dev' >> ~/lab_configs/$platform/$host.out 2> /dev/null"
  else
    eval "sush2 -n netops@$host 'sudo wedge_qsfp_util --direct-i2c' --reason 'dev' >> ~/lab_configs/$platform/$host.out 2> /dev/null"
  fi
}
export -f get_wedge_dump

for platform in "${platforms[@]}"; do
  pids=()
  eval "mkdir -p ~/lab_configs/$platform/"
  echo "Querying all $platform lab devices."
  while read -r host_and_config; do
    timeout "$command_execution_time" bash -c "get_wedge_dump $platform $host_and_config" &
    pids+=($!)
  done < <(eval "python3 fetch_platform_lab_devices.py $platform")
  wait "${pids[@]}"
  echo "Completed querying all $platform lab devices."
done
