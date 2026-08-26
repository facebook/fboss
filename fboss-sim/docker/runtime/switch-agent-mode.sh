#!/bin/bash
# Switch between monolithic and split agent modes at runtime

set -e

MODE="${1:-}"

usage() {
  cat <<EOF
Usage: $0 <mode>

Switch between FBOSS agent modes:
  mono   - Monolithic mode (wedge_agent only)
  split  - Split mode (fboss_sw_agent + fboss_hw_agent)

Current mode:
EOF
  if systemctl is-enabled wedge_agent.service >/dev/null 2>&1; then
    echo "  ✓ Monolithic (wedge_agent)"
  elif systemctl is-enabled fboss_sw_agent.service >/dev/null 2>&1; then
    echo "  ✓ Split (fboss_sw_agent + fboss_hw_agent)"
  else
    echo "  ✗ Unknown (no agent enabled)"
  fi
  echo ""
  echo "Active services:"
  systemctl list-units --type=service --state=running | grep -E "(wedge_agent|fboss_sw_agent|fboss_hw_agent)" || echo "  (none)"
}

switch_to_mono() {
  echo "🔄 Switching to monolithic mode..."

  # Stop split agents if running
  systemctl stop fboss_sw_agent.service 2>/dev/null || true
  systemctl stop fboss_hw_agent@0.service 2>/dev/null || true

  # Disable and mask split agents
  systemctl disable fboss_sw_agent.service 2>/dev/null || true
  systemctl disable fboss_hw_agent@0.service 2>/dev/null || true
  systemctl mask fboss_sw_agent.service 2>/dev/null || true
  systemctl mask fboss_hw_agent@0.service 2>/dev/null || true

  # Switch to mono config
  echo "  → Installing mono agent config..."
  cp /root/config/mono.conf /etc/coop/agent.conf

  # Unmask and enable monolithic agent
  systemctl unmask wedge_agent.service 2>/dev/null || true
  systemctl enable wedge_agent.service

  # Start monolithic agent
  systemctl start wedge_agent.service

  echo "✅ Switched to monolithic mode"
  echo ""
  systemctl status wedge_agent.service --no-pager
}

switch_to_split() {
  echo "🔄 Switching to split mode..."

  # Stop monolithic agent if running
  systemctl stop wedge_agent.service 2>/dev/null || true

  # Disable and mask monolithic agent
  systemctl disable wedge_agent.service 2>/dev/null || true
  systemctl mask wedge_agent.service 2>/dev/null || true

  # Switch to split config
  echo "  → Installing split agent config..."
  cp /root/config/split.conf /etc/coop/agent.conf

  # Unmask and enable split agents
  systemctl unmask fboss_sw_agent.service 2>/dev/null || true
  systemctl unmask fboss_hw_agent@0.service 2>/dev/null || true
  systemctl enable fboss_sw_agent.service
  systemctl enable fboss_hw_agent@0.service

  # Start split agents (HW agent first, then SW agent)
  systemctl start fboss_hw_agent@0.service
  sleep 2
  systemctl start fboss_sw_agent.service

  echo "✅ Switched to split mode"
  echo ""
  systemctl status fboss_sw_agent.service fboss_hw_agent@0.service --no-pager
}

case "$MODE" in
mono | monolithic)
  switch_to_mono
  ;;
split | multi)
  switch_to_split
  ;;
status | "")
  usage
  ;;
*)
  echo "❌ Error: Invalid mode '$MODE'"
  echo ""
  usage
  exit 1
  ;;
esac
