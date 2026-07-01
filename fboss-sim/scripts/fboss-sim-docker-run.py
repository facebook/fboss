#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

"""
Run fboss-sim runtime container with proper configuration.

This script launches the minimal fboss-sim runtime container created by
fboss-sim-docker-package.py with all necessary flags and capabilities.

Features:
- Proper systemd support (--cgroupns=host)
- Shared memory configuration (--shm-size=512m) to prevent malloc corruption
- Network capabilities for interface management
- IPv6-enabled Docker network so Thrift servers bind to :: (not 0.0.0.0)
- Support for monolithic and split agent modes
- Automatically stops and removes existing container
"""

import getpass
import hashlib
import subprocess
import sys

USERNAME = getpass.getuser()
DEFAULT_IMAGE_NAME = f"fboss_sim_runtime_{USERNAME}"
DEFAULT_CONTAINER_NAME = f"fboss_sim_runtime_{USERNAME}"


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
        capture_output=True, check=False,
    )
    if result.returncode == 0:
        print(f"  → Network {network_name} already exists")
        return
    print(f"  → Creating IPv6-enabled network {network_name}...")
    result = subprocess.run(
        ["docker", "network", "create", "--ipv6",
         f"--subnet={NETWORK_SUBNET_V6}", network_name],
        check=False, capture_output=True, text=True,
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
        subprocess.run(["docker", "stop", container_name], check=False, capture_output=True)
        print("  ✓ Container stopped")
        subprocess.run(["docker", "rm", container_name], check=False, capture_output=True)
        print("  ✓ Container removed")
    else:
        print("  → No existing container found")


def run_container() -> int:
    image_tag = f"{DEFAULT_IMAGE_NAME}:latest"
    print("\n🚀 Starting fboss-sim runtime container...")
    print(f"  Image:     {image_tag}")
    print(f"  Container: {DEFAULT_CONTAINER_NAME}")
    print(f"  Network:   {NETWORK_NAME} (IPv6-enabled)")

    cmd = [
        "docker", "run", "-d",
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
        "-v", "/sys/fs/cgroup:/sys/fs/cgroup:rw",
        "--tmpfs", "/run",
        "--tmpfs", "/tmp",
        "--name", DEFAULT_CONTAINER_NAME,
        image_tag,
    ]

    result = subprocess.run(cmd, check=False, capture_output=True, text=True)
    if result.returncode != 0:
        print("❌ Failed to start container")
        if result.stderr:
            print(f"Error: {result.stderr}")
        return result.returncode

    container_id = result.stdout.strip()
    print(f"  ✓ Container started (ID: {container_id[:12]})")
    return 0


def main() -> int:
    print("=" * 60)
    print("fboss-sim Runtime Container Launcher")
    print("=" * 60)

    print("\n🔍 Checking for existing container...")
    stop_existing_container(DEFAULT_CONTAINER_NAME)

    print("\n🌐 Setting up network...")
    ensure_network(NETWORK_NAME)

    ret = run_container()
    if ret != 0:
        return ret

    print(f"\n{'=' * 60}")
    print("✅ Container started successfully!")
    print(f"{'=' * 60}")
    print(f"\nContainer name: {DEFAULT_CONTAINER_NAME}")
    print("Agent mode:     split (fboss_sw_agent + fboss_hw_agent)")
    print("\nUseful commands:")
    print(f"  • SW agent status:  docker exec {DEFAULT_CONTAINER_NAME} systemctl status fboss_sw_agent")
    print(f"  • HW agent status:  docker exec {DEFAULT_CONTAINER_NAME} systemctl status fboss_hw_agent@0")
    print(f"  • View logs:        docker logs {DEFAULT_CONTAINER_NAME}")
    print(f"  • Enter shell:      docker exec -it {DEFAULT_CONTAINER_NAME} bash")
    print(f"  • Switch to mono:   docker exec {DEFAULT_CONTAINER_NAME} switch-agent-mode.sh mono")
    print(f"  • Run CLI test:     docker exec {DEFAULT_CONTAINER_NAME} /opt/fboss/bin/fboss2_integration_test")
    print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
