#!/bin/bash
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#
# Collect a vendor-escalation package for one AgentHwTest run:
#   - SAI Replayer log(s)         (--enable_replayer --sai_log; optional packet/get-attr logs)
#   - hw_config file(s)           (/dev/shm/fboss/hw_config[_idxN], snapshotted early)
#   - the raw test log
#
# Vendor-agnostic (Broadcom / Cisco-Leaba / NVIDIA): the replayer + hw_config are the SAI
# interface and the SOC/init config, which every SAI vendor can consume. Run twice (e.g. two
# switch_ids or two ports) to build an A/B differential package.
#
# Usage:
#   collect_vendor_escalation.sh <mono|multi> <hw_agent_or_-> <test_binary> <config> \
#       <gtest_filter> <switch_id> <out_dir> <pkt_log:0|1> [get_attr_log:0|1] [suffix]
#
# Examples:
#   # Broadcom multi-switch, capture switch_id 4, log packet-send SAI calls:
#   collect_vendor_escalation.sh multi \
#     /root/u/fboss_hw_agent-brcm-14.2.0.0_dnx_odp /root/u/multi_switch_agent_hw_test \
#     /root/u/janga800bic_multi.agent.materialized_JSON \
#     AgentMgmtPortL3ForwardingTest.VerifyMacRewrite 4 /root/u/escalation 1 0 sid4
#
#   # mono (single ASIC), no packet log:
#   collect_vendor_escalation.sh mono - \
#     /root/u/sai_agent_hw_test-brcm-14.2.0.0_dnx_odp /root/u/platform.agent.materialized_JSON \
#     AgentSomeTest.SomeCase 0 /root/u/escalation 0 0 run0

set -u

MODE="$1"          # mono | multi
HW_AGENT="$2"      # hw_agent binary (multi); "-" for mono
TEST_BINARY="$3"
CONFIG="$4"
FILTER="$5"
SWITCH_ID="$6"
OUT="$7"
PKT_LOG="${8:-0}"
GET_ATTR_LOG="${9:-0}"
SUF="${10:-run}"
SELF_PID=$$

mkdir -p "$OUT"

# Optional replayer flags (applied to the SAI-owning process). Array avoids word-split issues.
EXTRA=()
[ "$PKT_LOG" = "1" ] && EXTRA+=(--enable_packet_log)
[ "$GET_ATTR_LOG" = "1" ] && EXTRA+=(--enable_get_attr_log)

kill_procs() {
  local sig="${1:--TERM}"
  # Exclude this script (SELF_PID) and its parent shell (PPID): a wrapper whose command
  # line contains a matched binary name (e.g. the path passed as an argument) must not be
  # killed. pgrep -f matches on the full command line, so such ancestors can match a pattern.
  for pat in fboss_hw_agent multi_switch_agent_hw_test sai_agent_hw_test wedge_agent fboss_sw_agent; do
    pgrep -f "$pat" 2>/dev/null | grep -vxF -e "$SELF_PID" -e "$PPID" | xargs -r kill "$sig" 2>/dev/null
  done
}

snapshot_hw_config() {
  # Snapshot hw_config early (written at create_switch, before teardown can remove it).
  local i
  if [ "$MODE" = "mono" ]; then
    for ((i = 0; i < 300; i++)); do [ -f /dev/shm/fboss/hw_config ] && break; sleep 1; done
    cp -f /dev/shm/fboss/hw_config "$OUT/hw_config_${SUF}" 2>/dev/null && echo "[${SUF}] saved hw_config"
  else
    for ((i = 0; i < 300; i++)); do
      [ -f /dev/shm/fboss/hw_config_idx0 ] && [ -f /dev/shm/fboss/hw_config_idx1 ] && break
      sleep 1
    done
    cp -f /dev/shm/fboss/hw_config_idx0 "$OUT/hw_config_${SUF}_idx0" 2>/dev/null && echo "[${SUF}] saved hw_config_idx0"
    cp -f /dev/shm/fboss/hw_config_idx1 "$OUT/hw_config_${SUF}_idx1" 2>/dev/null && echo "[${SUF}] saved hw_config_idx1"
  fi
}

# --- Clean state ---
kill_procs
sleep 1
kill_procs -9
sleep 2
rm -rf /dev/shm/fboss /tmp/memory_snapshot_for_hw_agent_*
rm -f /var/facebook/fboss/agent_ensemble/hw_agent_test_0.conf \
  /var/facebook/fboss/agent_ensemble/hw_agent_test_1.conf \
  /var/facebook/fboss/agent_ensemble/agent.conf \
  /var/facebook/fboss/agent_ensemble/overridden_agent.conf

TEST_LOG="/tmp/escalation_test_${SUF}.log"

if [ "$MODE" = "mono" ]; then
  REP="$OUT/sai_replayer_${SUF}.log"
  rm -f "$REP"
  echo "=== [${SUF}] mono run: ${FILTER} ==="
  # Mono test owns SAI in-process: replayer flags go on the test binary.
  "$TEST_BINARY" --config "$CONFIG" --gtest_filter="$FILTER" \
    --enable_replayer --sai_log "$REP" "${EXTRA[@]}" &>"$TEST_LOG" &
  TPID=$!
  snapshot_hw_config
  wait "$TPID"
  TRC=$?
  echo "[${SUF}] TEST_DONE rc=$TRC"
  sleep 3
  kill_procs
  sleep 2
  kill_procs -9
  echo "[${SUF}] replayer:"
  wc -l "$REP" 2>/dev/null
else
  REP0="$OUT/sai_replayer_${SUF}_idx0.log"
  REP1="$OUT/sai_replayer_${SUF}_idx1.log"
  rm -f "$REP0" "$REP1"
  echo "=== [${SUF}] multi run: ${FILTER} (switch_id=${SWITCH_ID}) ==="
  # Start test binary first so both hw_agents discover their configs together.
  "$TEST_BINARY" --config "$CONFIG" \
    --hw_agent_for_testing --multi_npu_platform_mapping \
    --update_watermark_stats_interval_s=0 --update_voq_stats_interval_s=0 \
    --multi_switch --exit_for_any_hw_disconnect \
    --switch_id_for_testing="$SWITCH_ID" --gtest_filter="$FILTER" &>"$TEST_LOG" &
  TPID=$!
  for ((i = 0; i < 120; i++)); do
    [ -f /var/facebook/fboss/agent_ensemble/hw_agent_test_0.conf ] &&
      [ -f /var/facebook/fboss/agent_ensemble/hw_agent_test_1.conf ] && break
    kill -0 "$TPID" 2>/dev/null || {
      echo "[${SUF}] test exited early"
      break
    }
    sleep 1
  done
  # Replayer flags go on the hw_agents (they own SAI in multi mode).
  "$HW_AGENT" --config "$CONFIG" --switchIndex 0 \
    --hw_agent_for_testing --multi_npu_platform_mapping \
    --update_watermark_stats_interval_s=0 --update_voq_stats_interval_s=0 \
    --enable_replayer --sai_log "$REP0" "${EXTRA[@]}" &>"/tmp/escalation_hw0_${SUF}.log" &
  HW0=$!
  "$HW_AGENT" --config "$CONFIG" --switchIndex 1 \
    --hw_agent_for_testing --multi_npu_platform_mapping \
    --update_watermark_stats_interval_s=0 --update_voq_stats_interval_s=0 \
    --enable_replayer --sai_log "$REP1" "${EXTRA[@]}" &>"/tmp/escalation_hw1_${SUF}.log" &
  HW1=$!
  snapshot_hw_config
  wait "$TPID"
  TRC=$?
  echo "[${SUF}] TEST_DONE rc=$TRC"
  sleep 5
  # Graceful stop so the replayer logs flush before we read them.
  kill -TERM "$HW0" "$HW1" 2>/dev/null
  sleep 6
  kill -9 "$HW0" "$HW1" 2>/dev/null
  sleep 2
  echo "[${SUF}] replayer (idx0=switchIndex0, idx1=switchIndex1):"
  wc -l "$REP0" "$REP1" 2>/dev/null
fi

echo "=== [${SUF}] artifacts in $OUT ==="
ls -la "$OUT"
echo "=== [${SUF}] md5 ==="
md5sum "$OUT"/*"${SUF}"* 2>/dev/null
echo "ESCALATION_DONE suf=${SUF} test_rc=${TRC}"
