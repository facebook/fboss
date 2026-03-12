#!/usr/bin/env python3

import argparse

from neteng.fboss.ctrl import FbossCtrl
from thrift.protocol.THeaderProtocol import THeaderProtocol
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.transport.TSocket import TSocket


def get_interface_phy_state(
    host: str = "susw001.r001.c053.snc1",
    port: int = 5909,
    timeout: float = 10.0,
) -> dict[int, bool]:
    """
    Connect to a Thrift endpoint and get port status.
    Returns a dict mapping port ID to state (True=up, False=down).
    """
    socket = TSocket(host, port)
    socket.setTimeout(int(timeout * 1000))
    transport = THeaderTransport(socket)
    protocol = THeaderProtocol(transport)

    try:
        transport.open()
        client = FbossCtrl.Client(protocol)  # pyre-ignore[45]

        port_statuses = client.getPortStatus([])

        results = {}
        for port_id, status in port_statuses.items():
            results[port_id] = status.up

        return results
    finally:
        transport.close()


def get_interface_phy_state_with_names(
    host: str = "susw001.r001.c053.snc1",
    port: int = 5909,
    timeout: float = 10.0,
) -> dict[str, bool]:
    """
    Connect to a Thrift endpoint and get port status with port names.
    Returns a dict mapping port name to state (True=up, False=down).
    """
    socket = TSocket(host, port)
    socket.setTimeout(int(timeout * 1000))
    transport = THeaderTransport(socket)
    protocol = THeaderProtocol(transport)

    try:
        transport.open()
        client = FbossCtrl.Client(protocol)  # pyre-ignore[45]

        port_statuses = client.getPortStatus([])
        port_infos = client.getAllPortInfo()

        results = {}
        for port_id, status in port_statuses.items():
            port_name = port_infos.get(port_id)
            if port_name:
                results[port_name.name] = status.up
            else:
                results[str(port_id)] = status.up

        return results
    finally:
        transport.close()


def get_interface_phy_state_detailed(
    host: str = "susw001.r001.c053.snc1",
    port: int = 5909,
    timeout: float = 10.0,
) -> None:
    """Print detailed PHY info for debugging."""
    socket = TSocket(host, port)
    socket.setTimeout(int(timeout * 1000))
    transport = THeaderTransport(socket)
    protocol = THeaderProtocol(transport)

    try:
        transport.open()
        client = FbossCtrl.Client(protocol)  # pyre-ignore[45]

        phy_infos = client.getAllInterfacePhyInfo()

        for interface_name, phy_info in sorted(phy_infos.items())[:5]:
            print(f"\n=== {interface_name} ===")
            print(f"  name: {phy_info.name}")
            print(f"  speed: {phy_info.speed}")
            print(f"  linkState (deprecated): {phy_info.linkState}")
            if phy_info.state:
                print(f"  state.name: {phy_info.state.name}")
                print(f"  state.linkState: {phy_info.state.linkState}")
                print(f"  state.speed: {phy_info.state.speed}")
    finally:
        transport.close()


def main():
    parser = argparse.ArgumentParser(
        description="Get interface PHY state from FBOSS switch"
    )
    parser.add_argument(
        "--host", default="susw004.r147.c081.dkl6", help="Switch hostname"
    )
    parser.add_argument("--port", type=int, default=5909, help="Thrift port")
    args = parser.parse_args()

    print(f"Connecting to {args.host}:{args.port}...")
    print("\n--- All Interface States (using getPortStatus) ---")
    interface_states = get_interface_phy_state_with_names(args.host, args.port)

    for interface_name, state in sorted(interface_states.items()):
        print(f"{interface_name}: {state}")


if __name__ == "__main__":
    main()
