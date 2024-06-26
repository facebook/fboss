#!/bin/bash
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

set -e  # Exit immediately if any command exits with non-zero status

my_platforms=(DARWIN ELBERT FUJI MERU800BIA MINIPACK WEDGE100 WEDGE100S WEDGE400 WEDGE400C YAMP)
my_tiers=(fa fdsw fsw ma rdsw rgsw rsw rtsw rusw ssw xsw)
box_divider="BOX_DIVIDER"
cmd_divider="CMD_DIVIDER"
clear_files=true # determines whether or not each platform file is emptied before writing new output
num_iterations=1 # indicates the number of random boxes you want to query

get_tcvr_info() {
  tgt=$1
  platform=$2
  eval "echo '$box_divider' >> ~/tcvr_configs/$platform.out"
  eval "sush netops@$tgt sudo wedge_qsfp_util --reason 'dev' >> ~/tcvr_configs/$platform.out"
  eval "echo '$cmd_divider' >> ~/tcvr_configs/$platform.out"
  eval "fboss2 -H $tgt show transceiver >> ~/tcvr_configs/$platform.out"
  eval "echo '$cmd_divider' >> ~/tcvr_configs/$platform.out"
  eval "fboss2 -H $tgt show port >> ~/tcvr_configs/$platform.out"
}


for platform in "${my_platforms[@]}"; do
  if [ "$clear_files" = true ] ; then
    eval "truncate -s 0 ~/tcvr_configs/$platform.out"
  fi
  for tier in "${my_tiers[@]}"; do
    for _ in $(seq 1 $num_iterations); do
      tgt=$(eval "netwhoami randbox hw=$platform,smc_tier=fboss.agent.$tier")
      if [ -z "$tgt" ]; then
        echo "No device found for platform: $platform and tier: $tier. Skipping..."
        break
      fi
      echo "Selected $tgt for platform: $platform."
      get_tcvr_info "$tgt" "$platform"
    done
  done
done
