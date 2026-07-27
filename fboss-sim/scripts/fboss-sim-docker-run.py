#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

"""
Run fboss-sim runtime container with proper configuration.

Two modes:

- Default (production-like): launch the minimal runtime image built by
  fboss-sim-docker-package.py. Binaries + libs are baked into the image at
  /opt/fboss.

- --local (fast dev loop): skip the collect/package/image-build entirely.
  Launch the same systemd environment from the getdeps *build* image with the
  repo and the build scratch bind-mounted, symlink the freshly built binaries
  into /opt/fboss/bin, and bring the agents up as systemd services. Rebuild
  binaries and just restart the agents to iterate — no image rebuild.

Both modes give a production-like systemd container (PID 1 = /sbin/init,
agents run as systemd units), which is required for the config-mutation
fboss2_integration_test cases (they restart agents via `systemctl`).

Features:
- Proper systemd support (--cgroupns=host, /sbin/init as PID 1)
- Shared memory configuration (--shm-size=512m) to prevent malloc corruption
- Network capabilities for interface management
- IPv6-enabled Docker network so Thrift servers bind to :: (not 0.0.0.0)
- Support for monolithic and split agent modes
- Automatically stops and removes existing container
"""

import argparse
import getpass
import hashlib
import os
import subprocess
import sys
from pathlib import Path

USERNAME = getpass.getuser()
DEFAULT_IMAGE_NAME = f"fboss_sim_runtime_{USERNAME}"
DEFAULT_CONTAINER_NAME = f"fboss_sim_runtime_{USERNAME}"

# In --local mode we run from the getdeps build image (has the toolchain +
# system libs) instead of the packaged runtime image.
DEFAULT_BUILD_IMAGE = "fboss_docker:latest"


def user_subnet_v6(username: str) -> str:
    """Derive a per-user /64 ULA subnet from a stable hash of the username.

    The subnet must be unique per user: Docker rejects a `network create` whose
    pool overlaps an existing one, so two users sharing a host would otherwise
    collide on a hardcoded subnet. We keep `fd00:fb05` as the identifying
    fboss-sim prefix and fill the next two hextets (32 bits, ~4 billion buckets)
    from a hash, making collisions between users vanishingly unlikely. SHA-256
    rather than the builtin hash() so the value is stable across processes
    (hash() is randomized per interpreter run via PYTHONHASHSEED).
    """
    digest = hashlib.sha256(username.encode()).digest()
    hextet1 = (digest[0] << 8) | digest[1]
    hextet2 = (digest[2] << 8) | digest[3]
    return f"fd00:fb05:{hextet1:x}:{hextet2:x}::/64"


# User-defined network with IPv6 enabled.
# Docker sets net.ipv6.conf.eth0.disable_ipv6=1 on the default bridge, which
# causes folly/Thrift (using getaddrinfo+AI_ADDRCONFIG) to bind 0.0.0.0 only.
# A user-defined network with --ipv6 avoids that and gives the container a real
# non-loopback IPv6 address, so AI_ADDRCONFIG returns AF_INET6 results.
NETWORK_NAME = f"fboss_sim_net_{USERNAME}"
NETWORK_SUBNET_V6 = user_subnet_v6(USERNAME)


def ensure_network(network_name: str) -> None:
    result = subprocess.run(
        ["docker", "network", "inspect", network_name],
        capture_output=True,
        check=False,
    )
    if result.returncode == 0:
        print(f"  → Network {network_name} already exists")
        return
    print(f"  → Creating IPv6-enabled network {network_name}...")
    result = subprocess.run(
        [
            "docker",
            "network",
            "create",
            "--ipv6",
            f"--subnet={NETWORK_SUBNET_V6}",
            network_name,
        ],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode == 0:
        print(f"  ✓ Network created ({NETWORK_SUBNET_V6})")
    else:
        print(f"  ❌ Network creation failed: {result.stderr.strip()}")
        sys.exit(1)


def stop_existing_container(container_name: str) -> None:
    check_cmd = ["docker", "ps", "-a", "-q", "-f", f"name={container_name}"]
    result = subprocess.run(check_cmd, check=False, capture_output=True, text=True)
    if result.stdout.strip():
        print("  → Found existing container, stopping...")
        subprocess.run(
            ["docker", "stop", container_name], check=False, capture_output=True
        )
        print("  ✓ Container stopped")
        subprocess.run(
            ["docker", "rm", container_name], check=False, capture_output=True
        )
        print("  ✓ Container removed")
    else:
        print("  → No existing container found")


def teardown() -> int:
    """Stop + remove the sim container (gracefully) and its IPv6 network."""
    print("\n🧹 Tearing down fboss-sim container + network...")
    # stop_existing_container does `docker stop` (STOPSIGNAL SIGRTMIN+3 -> clean
    # systemd shutdown) then `docker rm`.
    stop_existing_container(DEFAULT_CONTAINER_NAME)
    result = subprocess.run(
        ["docker", "network", "rm", NETWORK_NAME],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode == 0:
        print(f"  ✓ Network {NETWORK_NAME} removed")
    else:
        print(f"  → Network {NETWORK_NAME}: {result.stderr.strip() or 'not found'}")
    print("✓ Teardown complete")
    return 0


def _base_run_flags() -> list[str]:
    """Systemd + networking + capability flags shared by both modes."""
    return [
        "-d",
        # CAP_NET_ADMIN: TunManager network interface creation
        # CAP_SYS_ADMIN: systemd cgroup management
        "--cap-add=NET_ADMIN",
        "--cap-add=SYS_ADMIN",
        "--device=/dev/net/tun",
        # IPv6-enabled user-defined network (avoids Docker default bridge
        # setting net.ipv6.conf.eth0.disable_ipv6=1)
        f"--network={NETWORK_NAME}",
        "--shm-size=512m",
        "--memory=4g",
        "--cgroupns=host",
        "-v",
        "/sys/fs/cgroup:/sys/fs/cgroup:rw",
        "--tmpfs",
        "/run",
        "--tmpfs",
        "/tmp",
    ]


def run_container() -> int:
    """Production-like mode: run the packaged runtime image (systemd baked in)."""
    image_tag = f"{DEFAULT_IMAGE_NAME}:latest"
    print("\n🚀 Starting fboss-sim runtime container...")
    print(f"  Image:     {image_tag}")
    print(f"  Container: {DEFAULT_CONTAINER_NAME}")
    print(f"  Network:   {NETWORK_NAME} (IPv6-enabled)")

    cmd = (
        ["docker", "run"]
        + _base_run_flags()
        + ["--name", DEFAULT_CONTAINER_NAME, image_tag]
    )

    result = subprocess.run(cmd, check=False, capture_output=True, text=True)
    if result.returncode != 0:
        print("❌ Failed to start container")
        if result.stderr:
            print(f"Error: {result.stderr}")
        return result.returncode

    container_id = result.stdout.strip()
    print(f"  ✓ Container started (ID: {container_id[:12]})")
    return 0


# --- Local (fast dev) mode -------------------------------------------------
#
# Bring-up run inside a fresh container launched from the build image with
# PID 1 = /sbin/init (a new, dedicated sim container — not the getdeps build
# container). Mirrors what Dockerfile.runtime does at image-build time, but
# symlinks the freshly built binaries from the mounted build tree instead of
# copying them, so a rebuild + `systemctl restart` is all that's needed to
# iterate.
#
# NOTE on RUNPATH: getdeps bakes an absolute RUNPATH (the build scratch path)
# into the binaries, so the build scratch must appear at that exact path inside
# the container. We bind-mount --build-dir at --runpath-dir for that reason;
# for a native (host) build the two are identical.
_LOCAL_SETUP = r"""
set -e
REPO="{repo}"
B="{runpath}/build/fboss"
ROOTFS="$REPO/fboss-image/image_builder/templates/centos-09.0/root_files"

echo "→ Installing runtime deps..."
dnf install -y iproute procps-ng jemalloc jq >/dev/null

mkdir -p /opt/fboss/bin /opt/fboss/lib /opt/fboss/share /etc/coop /root/config \
         /var/facebook/fboss /var/facebook/logs/fboss /dev/shm/fboss

echo "→ Installing systemd units + rootfs overlay..."
cp -a "$ROOTFS"/. /

echo "→ Symlinking freshly built binaries into /opt/fboss/bin..."
for f in wedge_agent-sai_impl fboss_sw_agent fboss_hw_agent-sai_impl \
         fboss2 fboss2-dev fboss2_integration_test; do
  ln -sfn "$B/$f" "/opt/fboss/bin/$f"
done
ln -sfn "$REPO/fboss/oss/scripts/run_scripts/setup_fboss_env" \
        /opt/fboss/bin/setup_fboss_env

echo "→ Installing configs + fruid..."
cp "$REPO/fboss-sim/docker/runtime/mono.conf" /root/config/mono.conf
jq '.defaultCommandLineArgs.multi_switch="true"' \
   /root/config/mono.conf > /root/config/split.conf
cp "$REPO/fboss-sim/docker/runtime/fruid.json" /var/facebook/fboss/fruid.json

echo "→ Running setup-container.sh (jemalloc, service enable, /etc/coop git)..."
cp "$REPO/fboss-sim/docker/runtime/setup-container.sh" /tmp/setup-container.sh
chmod +x /tmp/setup-container.sh
/tmp/setup-container.sh

# Disable host TUN for sw_agent: TunManager isn't needed for the sim's CLI /
# config testing and crashes under fake SAI (getTableId->l3SwitchType).
# mono.conf/split.conf also set tun_intf=false; this drop-in enforces it.
echo "→ Applying --tun_intf=false drop-in for fboss_sw_agent..."
mkdir -p /etc/systemd/system/fboss_sw_agent.service.d
cat > /etc/systemd/system/fboss_sw_agent.service.d/override.conf <<'DROPIN'
[Service]
ExecStart=
ExecStart=/bin/bash -c 'source /opt/fboss/bin/setup_fboss_env && exec /opt/fboss/bin/fboss_sw_agent --fsdb_client_ssl_preferred=false --thrift_ssl_policy=disabled --tun_intf=false'
DROPIN
systemctl daemon-reload

# Neither switch-agent-mode.sh nor a wedge_agent.service unit ship in the
# centos-09.0 overlay, so mono mode and live mode-switching don't work out of
# the box. Install the mode switcher and synthesize a wedge_agent.service
# (running wedge_agent-sai_impl) so `switch-agent-mode.sh mono|split` works on a
# live container. setup-container.sh masks wedge_agent by default (split stays
# the default); switch-agent-mode.sh unmasks it when switching to mono.
echo "→ Installing switch-agent-mode.sh + wedge_agent.service (mono)..."
cp "$REPO/fboss-sim/docker/runtime/switch-agent-mode.sh" \
   /usr/local/bin/switch-agent-mode.sh
chmod +x /usr/local/bin/switch-agent-mode.sh
cat > /usr/lib/systemd/system/wedge_agent.service <<'UNIT'
[Unit]
Description=FBOSS Wedge Agent (mono)
StartLimitIntervalSec=0
[Service]
Type=simple
LimitNOFILE=10000000
Environment="PATH=/opt/fboss/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin"
Environment="LD_LIBRARY_PATH=/opt/fboss/lib"
Environment="LD_PRELOAD=/usr/lib64/libjemalloc.so.2"
WorkingDirectory=/opt/fboss
ExecStart=/bin/bash -c 'source /opt/fboss/bin/setup_fboss_env && exec /opt/fboss/bin/wedge_agent-sai_impl --fsdb_client_ssl_preferred=false --thrift_ssl_policy=disabled --tun_intf=false'
Restart=always
StandardOutput=append:/var/facebook/logs/fboss/wedge_agent.log
StandardError=append:/var/facebook/logs/fboss/wedge_agent.log
[Install]
WantedBy=multi-user.target
UNIT
systemctl daemon-reload
echo "✓ local setup complete"
"""


def _docker_exec(container: str, script: str) -> int:
    return subprocess.run(
        ["docker", "exec", container, "bash", "-c", script], check=False
    ).returncode


def run_local(args: argparse.Namespace) -> int:
    repo = os.path.abspath(args.repo_dir)
    build_dir = os.path.abspath(args.build_dir)
    runpath = args.runpath_dir or build_dir
    if not os.path.isdir(os.path.join(build_dir, "build", "fboss")):
        print(f"❌ No build/fboss under --build-dir {build_dir}")
        print(
            "   Point --build-dir at your getdeps scratch (contains build/, installed/)."
        )
        return 1

    print("\n🚀 Starting fboss-sim LOCAL dev container (systemd)...")
    print(f"  Image:     {args.build_image}")
    print(f"  Container: {DEFAULT_CONTAINER_NAME}")
    print(f"  Repo:      {repo} -> /var/FBOSS/fboss")
    print(f"  Build:     {build_dir} -> {runpath}  (RUNPATH-matching)")
    print(f"  Mode:      {args.mode}")

    cmd = (
        ["docker", "run"]
        + _base_run_flags()
        + [
            "-v",
            f"{repo}:/var/FBOSS/fboss",
            "-v",
            f"{build_dir}:{runpath}",
            "--name",
            DEFAULT_CONTAINER_NAME,
            args.build_image,
            "/sbin/init",
        ]
    )
    result = subprocess.run(cmd, check=False, capture_output=True, text=True)
    if result.returncode != 0:
        print("❌ Failed to start container")
        if result.stderr:
            print(f"Error: {result.stderr}")
        return result.returncode
    print(f"  ✓ Container started (ID: {result.stdout.strip()[:12]})")

    print("\n🔧 Setting up sim environment inside the container...")
    setup = _LOCAL_SETUP.format(repo="/var/FBOSS/fboss", runpath=runpath)
    rc = _docker_exec(DEFAULT_CONTAINER_NAME, setup)
    if rc != 0:
        print("❌ In-container setup failed")
        return rc

    print(f"\n▶ Starting agents ({args.mode} mode)...")
    if args.mode == "mono":
        rc = _docker_exec(DEFAULT_CONTAINER_NAME, "switch-agent-mode.sh mono")
    else:
        rc = _docker_exec(
            DEFAULT_CONTAINER_NAME, "systemctl start fboss_hw_agent@0 fboss_sw_agent"
        )
    if rc != 0:
        print("❌ Failed to start agents")
        return rc
    return 0


def emit_github_output(key: str, value: str) -> None:
    """Expose a value to later GitHub Actions steps via $GITHUB_OUTPUT.

    The container name is derived from getpass.getuser(), which does not always
    match the shell's ${USER} in a CI `run:` step. Emitting it here lets the
    workflow reference the exact name this script used instead of re-deriving it.
    No-op outside GitHub Actions (when $GITHUB_OUTPUT is unset).
    """
    github_output = os.environ.get("GITHUB_OUTPUT")
    if not github_output:
        return
    with open(github_output, "a", encoding="utf-8") as fh:
        fh.write(f"{key}={value}\n")


def _find_repo_root() -> str:
    """Walk up from this script to the fboss repo root.

    Robust to the script's nesting depth (fboss-sim/scripts vs deeper): the repo
    root is the first ancestor that contains both `fboss-sim` and `fboss-image`.
    """
    start = Path(__file__).resolve().parent
    for cand in [start, *start.parents]:
        if (cand / "fboss-sim").is_dir() and (cand / "fboss-image").is_dir():
            return str(cand)
    return str(Path(__file__).resolve().parents[2])


def _parse_args() -> argparse.Namespace:
    default_repo = _find_repo_root()
    parser = argparse.ArgumentParser(description="Run the fboss-sim runtime container")
    parser.add_argument(
        "--local",
        action="store_true",
        help="Fast dev mode: run from the build image with the build tree "
        "bind-mounted (no runtime image build).",
    )
    parser.add_argument(
        "--mode",
        choices=["mono", "multi"],
        default="multi",
        help="Agent mode for --local (default: multi).",
    )
    parser.add_argument(
        "--build-dir",
        default=os.path.join(default_repo, ".build_dir"),
        help="[--local] Host path to the getdeps scratch (contains build/, "
        "installed/). Default: <repo>/.build_dir",
    )
    parser.add_argument(
        "--runpath-dir",
        default=None,
        help="[--local] In-container path to mount --build-dir at; must match "
        "the binaries' baked RUNPATH. Default: same as --build-dir.",
    )
    parser.add_argument(
        "--repo-dir",
        default=default_repo,
        help="[--local] Host path to the fboss repo root. Default: derived "
        "from this script's location.",
    )
    parser.add_argument(
        "--build-image",
        default=DEFAULT_BUILD_IMAGE,
        help=f"[--local] Build image to run. Default: {DEFAULT_BUILD_IMAGE}",
    )
    parser.add_argument(
        "--teardown",
        action="store_true",
        help="Stop and remove the sim container and its IPv6 network, then exit.",
    )
    return parser.parse_args()


def _print_usage_footer(is_mono: bool) -> None:
    c = DEFAULT_CONTAINER_NAME
    mode = (
        "mono (wedge_agent)" if is_mono else "split (fboss_sw_agent + fboss_hw_agent)"
    )
    other_mode = "split" if is_mono else "mono"
    print(f"\n{'=' * 60}")
    print("✅ Container started successfully!")
    print(f"{'=' * 60}")
    print(f"\nContainer name: {c}")
    print(f"Agent mode:     {mode}")
    print("\nUseful commands:")
    if is_mono:
        print(f"  • Agent status:     docker exec {c} systemctl status wedge_agent")
    else:
        print(f"  • SW agent status:  docker exec {c} systemctl status fboss_sw_agent")
        print(
            f"  • HW agent status:  docker exec {c} systemctl status fboss_hw_agent@0"
        )
    print(f"  • Switch mode:      docker exec {c} switch-agent-mode.sh {other_mode}")
    print(
        f"  • View logs:        docker exec {c} tail -f /var/facebook/logs/fboss/wedge_agent.log"
    )
    print(f"  • Enter shell:      docker exec -it {c} bash")
    print(
        f"  • Run CLI test:     docker exec {c} /opt/fboss/bin/fboss2_integration_test"
    )
    print()


def main() -> int:
    args = _parse_args()
    print("=" * 60)
    print("fboss-sim Runtime Container Launcher")
    print("=" * 60)

    if args.teardown:
        return teardown()

    print("\n🔍 Checking for existing container...")
    stop_existing_container(DEFAULT_CONTAINER_NAME)

    print("\n🌐 Setting up network...")
    ensure_network(NETWORK_NAME)

    if args.local:
        ret = run_local(args)
        is_mono = args.mode == "mono"
    else:
        ret = run_container()
        is_mono = False
    if ret != 0:
        return ret

    emit_github_output("container_name", DEFAULT_CONTAINER_NAME)
    _print_usage_footer(is_mono)
    return 0


if __name__ == "__main__":
    sys.exit(main())
