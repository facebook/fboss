#!/bin/bash
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

set -e  # Exit immediately if any command exits with non-zero status

role=rsw
platform=darwin
box_divider="BOX_DIVIDER"
cmd_divider="CMD_DIVIDER"
echo "Querying all Darwin hosts."

get_tcvr_info() {
  tgt=$1
  platform=$2
  eval "echo '$box_divider' >> ~/tcvr_configs/$platform.out"
  eval "sush2 netops@$tgt sudo wedge_qsfp_util --reason 'dev' >> ~/tcvr_configs/$platform.out"
  eval "echo '$cmd_divider' >> ~/tcvr_configs/$platform.out"
  eval "fboss2 -H $tgt show transceiver >> ~/tcvr_configs/$platform.out"
  eval "echo '$cmd_divider' >> ~/tcvr_configs/$platform.out"
  eval "fboss2 -H $tgt show port >> ~/tcvr_configs/$platform.out"
}

eval "mkdir -p ~/tcvr_configs/$platform/"
while read -r dc; do
    echo "SELECTED DC: $dc."
    while read -r pod; do
        echo "SELECTED POD: $pod."
        while read -r host; do
            echo "SELECTED HOST: $host."
            eval "truncate -s 0 ~/tcvr_configs/$platform/$host.out"
            get_tcvr_info "$host" "$platform/$host" &
        done < <(eval "smcc list-hosts $pod")
        wait
    done < <(eval "smcc list-children $dc")
    wait
done < <(eval "smcc list-children fboss.agent.$role.$platform")
wait
