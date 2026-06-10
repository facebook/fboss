#!/bin/bash
# Container setup script for FBOSS base image
# This script configures the container environment for FBOSS agents
set -ex

echo "=========================================="
echo "FBOSS Container Setup"
echo "=========================================="

# 1. Make scripts executable
echo "→ Making scripts executable..."
chmod +x /usr/local/bin/* /opt/fboss/bin/* 2>/dev/null || true

# 2. Set environment (source setup_fboss_env if it exists)
echo "→ Configuring environment..."
if [ -f /opt/fboss/bin/setup_fboss_env ]; then
  echo "source /opt/fboss/bin/setup_fboss_env" >>/etc/profile.d/fboss.sh
fi

# 3. Initialize git repo in /etc/coop for config management
echo "→ Initializing git repo in /etc/coop..."
cd /etc/coop
git init
git config user.email "fboss@container"
git config user.name "FBOSS Container"

# Install split config under cli/ and symlink agent.conf, matching the
# layout fboss2-dev produces after its first commit. Without this, sessions
# created on a fresh runtime capture an empty base SHA (no commits in the
# repo), which silently disables config-session conflict detection. See
# ConfigSession::initializeGit().
echo "→ Installing split agent config..."
mkdir -p /etc/coop/cli
cp /root/config/split.conf /etc/coop/cli/agent.conf
ln -sf cli/agent.conf /etc/coop/agent.conf

# Seed an initial commit so ConfigSession::initializeSession() captures a
# non-empty base_ on the very first session. Include an empty metadata file
# so `config rollback <initial-sha>` can read cli/cli_metadata.json from the
# revision (ConfigSession::rollback requires it).
echo '{"action":{},"commands":[],"base":""}' >/etc/coop/cli/cli_metadata.json
git add cli/agent.conf cli/cli_metadata.json agent.conf
git commit -m "Initial fboss-sim runtime config" --quiet

# 4. Configure services to use jemalloc instead of glibc malloc
# jemalloc is more robust against memory corruption issues in fake SAI
echo "→ Configuring jemalloc for all agent services..."
for service in wedge_agent.service fboss_sw_agent.service fboss_hw_agent@.service; do
  if [ -f /usr/lib/systemd/system/$service ]; then
    echo "  - Configuring $service"
    sed -i '/Environment="LD_LIBRARY_PATH/a Environment="LD_PRELOAD=/usr/lib64/libjemalloc.so.2"' \
      /usr/lib/systemd/system/$service
  fi
done

# 5. Reload systemd to pick up service changes
echo "→ Reloading systemd daemon..."
systemctl daemon-reload || true

# 6. Enable split mode (fboss_sw_agent + fboss_hw_agent) by default
echo "→ Enabling split mode (fboss_sw_agent + fboss_hw_agent) by default..."
mkdir -p /etc/systemd/system/multi-user.target.wants
ln -sf /usr/lib/systemd/system/fboss_sw_agent.service \
  /etc/systemd/system/multi-user.target.wants/fboss_sw_agent.service
# Template instance: symlink name is the instance (@0), but target is the template (@)
ln -sf /usr/lib/systemd/system/fboss_hw_agent@.service \
  /etc/systemd/system/multi-user.target.wants/fboss_hw_agent@0.service

# Verify the symlinks were created
if [ -L /etc/systemd/system/multi-user.target.wants/fboss_sw_agent.service ] &&
  [ -L /etc/systemd/system/multi-user.target.wants/fboss_hw_agent@0.service ]; then
  echo "✓ fboss_sw_agent enabled"
  echo "✓ fboss_hw_agent@0 enabled"
else
  echo "✗ Failed to enable split agent services"
  exit 1
fi

# 7. Mask monolithic agent service (can be unmasked at runtime with switch-agent-mode.sh)
echo "→ Masking monolithic agent service..."
systemctl mask wedge_agent.service || true

# Note: Other FBOSS services (platform_manager, qsfp_service, etc.) are not enabled
# by default, so they won't start. No need to explicitly mask them.

echo ""
echo "=========================================="
echo "✅ Container Setup Complete!"
echo "=========================================="
echo ""
echo "Configuration Summary:"
echo "  - Split mode: ENABLED (fboss_sw_agent + fboss_hw_agent)"
echo "  - Monolithic mode: DISABLED (masked, use switch-agent-mode.sh to enable)"
echo "  - Memory allocator: jemalloc (via LD_PRELOAD)"
echo "  - Multi-switch flag: ENABLED (via /etc/coop/agent.conf)"
echo ""
