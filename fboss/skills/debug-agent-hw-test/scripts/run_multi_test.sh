#!/bin/bash
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#
# Run a multi-switch AgentHwTest for one switch_id: cold boot then conditional warm boot.
# Starts hw_agent background processes, runs the test, then cleans up.
#
# Usage: bash run_multi_test.sh <hw_agent_binary> <test_binary> <config_path> <test_filter> <switch_id>
#
# Example:
#   bash run_multi_test.sh \
#     /root/skhare/fboss_hw_agent-brcm-12.2.0.0_dnx_odp \
#     /root/skhare/multi_switch_agent_hw_test \
#     /root/skhare/janga800bic_multi.agent.materialized_JSON \
#     AgentVoqSwitchTest.addRemoveNeighbor \
#     0

set -u

HW_AGENT_BINARY="$1"
TEST_BINARY="$2"
CONFIG="$3"
TEST_FILTER="$4"
SWITCH_ID="$5"
SELF_PID=$$

CB_LOG="/tmp/multi_switch_agent_hw_test_switch_id_${SWITCH_ID}_cb.log"
WB_LOG="/tmp/multi_switch_agent_hw_test_switch_id_${SWITCH_ID}_wb.log"

# Kill matching processes except this script
kill_procs() {
  local sig="${1:--TERM}"
  for pat in fboss_hw_agent multi_switch_agent_hw_test wedge_agent fboss_sw_agent; do
    pgrep -f "$pat" 2>/dev/null | grep -v "^${SELF_PID}$" | xargs -r kill "$sig" 2>/dev/null
  done
}

# --- Clean state ---
kill_procs
sleep 1
kill_procs -9
rm -rf /dev/shm/fboss
rm -rf /tmp/memory_snapshot_for_hw_agent_*

# Remove stale hw_agent config files to prevent timing skew.
# hw_agents poll for these files on startup. If a stale config exists from a
# previous run, that hw_agent starts initializing immediately while the other
# waits for the test binary to create its config. This causes one hw_agent to
# connect to SwSwitch much earlier, triggering expensive reinitStats on the
# late-connecting hw_agent's ports (disabled->enabled transition on accumulated
# state), which can cause LinkStateToggler timeouts.
rm -f /var/facebook/fboss/agent_ensemble/hw_agent_test_0.conf
rm -f /var/facebook/fboss/agent_ensemble/hw_agent_test_1.conf
rm -f /var/facebook/fboss/agent_ensemble/agent.conf
rm -f /var/facebook/fboss/agent_ensemble/overridden_agent.conf

# --- Start test binary first ---
# The test binary creates hw_agent config files and starts the SwSwitch thrift
# server. Starting it before hw_agents ensures both hw_agents discover their
# config files at the same time and initialize in parallel, connecting to
# SwSwitch close together.
echo "=== Cold boot: ${TEST_FILTER} (switch_id=${SWITCH_ID}) ==="
"${TEST_BINARY}" \
  --config "${CONFIG}" \
  --hw_agent_for_testing --multi_npu_platform_mapping \
  --update_watermark_stats_interval_s=0 --update_voq_stats_interval_s=0 \
  --multi_switch --exit_for_any_hw_disconnect \
  --switch_id_for_testing="${SWITCH_ID}" \
  --setup-for-warmboot \
  --gtest_filter="${TEST_FILTER}" \
  &> "${CB_LOG}" &
TEST_PID=$!

# Wait for both hw_agent config files to be created by the test binary
echo "Waiting for config files..."
WAIT_COUNT=0
MAX_WAIT=60
while [ $WAIT_COUNT -lt $MAX_WAIT ]; do
  if [ -f /var/facebook/fboss/agent_ensemble/hw_agent_test_0.conf ] && \
     [ -f /var/facebook/fboss/agent_ensemble/hw_agent_test_1.conf ]; then
    echo "Config files ready after ${WAIT_COUNT}s"
    break
  fi
  if ! kill -0 $TEST_PID 2>/dev/null; then
    echo "ERROR: Test binary exited before creating config files"
    wait $TEST_PID
    exit $?
  fi
  sleep 1
  WAIT_COUNT=$((WAIT_COUNT + 1))
done
if [ $WAIT_COUNT -ge $MAX_WAIT ]; then
  echo "ERROR: Timed out waiting for config files after ${MAX_WAIT}s"
  kill $TEST_PID 2>/dev/null
  exit 1
fi

# --- Start hw_agent processes ---
# Both hw_agents find their config files immediately and start initializing
# in parallel, reducing the connection time gap to SwSwitch.
echo "Starting hw_agent processes..."
# NOTE: --config is required for fbossCommonInit() even though
# getConfigFileForTesting() overrides it when --hw_agent_for_testing is set.
"${HW_AGENT_BINARY}" \
  --config "${CONFIG}" --switchIndex 0 \
  --hw_agent_for_testing --multi_npu_platform_mapping \
  --update_watermark_stats_interval_s=0 --update_voq_stats_interval_s=0 \
  --sai_log /var/facebook/logs/fboss/sdk/sai_replayer_0.log \
  &> /tmp/fboss_hw_agent_log_0.log &
HW_AGENT_PID_0=$!

"${HW_AGENT_BINARY}" \
  --config "${CONFIG}" --switchIndex 1 \
  --hw_agent_for_testing --multi_npu_platform_mapping \
  --update_watermark_stats_interval_s=0 --update_voq_stats_interval_s=0 \
  --sai_log /var/facebook/logs/fboss/sdk/sai_replayer_1.log \
  &> /tmp/fboss_hw_agent_log_1.log &
HW_AGENT_PID_1=$!

sleep 5

# Verify hw_agents are running
if ! kill -0 "$HW_AGENT_PID_0" 2>/dev/null || ! kill -0 "$HW_AGENT_PID_1" 2>/dev/null; then
  echo "ERROR: One or more hw_agent processes failed to start"
  echo "Check /tmp/fboss_hw_agent_log_0.log and /tmp/fboss_hw_agent_log_1.log"
  exit 1
fi

# Check if hw_agents are still alive after a test phase.
# If test binary passed but an hw_agent crashed, override result to FAILED.
check_hw_agent_status() {
  local phase="$1"
  local hw_agent_ok=true
  if ! kill -0 "$HW_AGENT_PID_0" 2>/dev/null; then
    echo "ERROR: hw_agent (switchIndex 0, pid ${HW_AGENT_PID_0}) died during ${phase}"
    hw_agent_ok=false
  fi
  if ! kill -0 "$HW_AGENT_PID_1" 2>/dev/null; then
    echo "ERROR: hw_agent (switchIndex 1, pid ${HW_AGENT_PID_1}) died during ${phase}"
    hw_agent_ok=false
  fi
  if [ "$hw_agent_ok" = false ]; then
    return 1
  fi
  return 0
}

# Wait for the cold boot test binary to finish
wait $TEST_PID
cb_exit=$?

# Check hw_agent status after cold boot (override to FAILED if hw_agent crashed)
if [ $cb_exit -eq 0 ] && ! check_hw_agent_status "cold boot"; then
  echo "Cold boot test binary passed but hw_agent exited abnormally"
  cb_exit=1
fi

# --- Warm boot (only if cold boot passed) ---
wb_exit=-1
if [ $cb_exit -eq 0 ]; then
  echo "=== Warm boot: ${TEST_FILTER} (switch_id=${SWITCH_ID}) ==="
  "${TEST_BINARY}" \
    --config "${CONFIG}" \
    --hw_agent_for_testing --multi_npu_platform_mapping \
    --update_watermark_stats_interval_s=0 --update_voq_stats_interval_s=0 \
    --multi_switch --exit_for_any_hw_disconnect \
    --switch_id_for_testing="${SWITCH_ID}" \
    --gtest_filter="${TEST_FILTER}" \
    &> "${WB_LOG}"
  wb_exit=$?

  # Check hw_agent status after warm boot
  if [ $wb_exit -eq 0 ] && ! check_hw_agent_status "warm boot"; then
    echo "Warm boot test binary passed but hw_agent exited abnormally"
    wb_exit=1
  fi
fi

# --- Cleanup ---
kill_procs
sleep 1
kill_procs -9
rm -rf /dev/shm/fboss

# --- Summary ---
echo ""
echo "=== Test Results ==="
if [ $cb_exit -eq 0 ]; then
  echo "Cold boot: PASSED (exit code 0)"
else
  echo "Cold boot: FAILED (exit code $cb_exit)"
fi
if [ $wb_exit -eq -1 ]; then
  echo "Warm boot: SKIPPED (cold boot failed)"
elif [ $wb_exit -eq 0 ]; then
  echo "Warm boot: PASSED (exit code 0)"
else
  echo "Warm boot: FAILED (exit code $wb_exit)"
fi
echo "=== Log Files ==="
echo "Test (cold boot): ${CB_LOG}"
if [ $wb_exit -ne -1 ]; then
  echo "Test (warm boot): ${WB_LOG}"
fi
echo "HW agent 0:       /tmp/fboss_hw_agent_log_0.log"
echo "HW agent 1:       /tmp/fboss_hw_agent_log_1.log"

# Exit with failure if any test failed
if [ $cb_exit -ne 0 ] || [ $wb_exit -gt 0 ]; then
  exit 1
fi
exit 0
