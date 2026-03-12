#!/usr/bin/env python3

import argparse
import json
from typing import Any

from neteng.fboss.ctrl import FbossCtrl
from thrift.protocol.THeaderProtocol import THeaderProtocol
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.transport.TSocket import TSocket


def create_client(host: str, port: int = 5909, timeout: float = 15.0):
    socket = TSocket(host, port)
    socket.setTimeout(int(timeout * 1000))
    transport = THeaderTransport(socket)
    protocol = THeaderProtocol(transport)
    transport.open()
    client = FbossCtrl.Client(protocol)  # pyre-ignore[45]
    return client, transport


def check_port_status(client) -> dict[str, Any]:
    port_statuses = client.getPortStatus([])
    port_infos = client.getAllPortInfo()

    results = {}
    up_count = 0
    down_count = 0

    for port_id, status in port_statuses.items():
        port_name = port_infos.get(port_id)
        name = port_name.name if port_name else str(port_id)
        results[name] = {
            "up": status.up,
            "enabled": status.enabled,
            "speed_mbps": status.speedMbps,
        }
        if status.up:
            up_count += 1
        else:
            down_count += 1

    return {
        "total_ports": len(results),
        "ports_up": up_count,
        "ports_down": down_count,
        "ports": results,
    }


def check_lldp_neighbors(client) -> dict[str, Any]:
    try:
        neighbors = client.getLldpNeighbors()
        neighbor_list = []
        for n in neighbors:
            neighbor_list.append(
                {
                    "local_port": n.localPort,
                    "remote_system": n.systemName,
                }
            )
        return {
            "count": len(neighbors),
            "neighbors": neighbor_list[:5],
        }
    except Exception as e:
        return {"error": str(e)}


def check_arp_table(client) -> dict[str, Any]:
    try:
        arp_entries = client.getArpTable()
        return {"count": len(arp_entries)}
    except Exception as e:
        return {"error": str(e)}


def check_ndp_table(client) -> dict[str, Any]:
    try:
        ndp_entries = client.getNdpTable()
        return {"count": len(ndp_entries)}
    except Exception as e:
        return {"error": str(e)}


def check_switch_state(client) -> dict[str, Any]:
    try:
        run_state = client.getSwitchRunState()
        return {"run_state": str(run_state)}
    except Exception as e:
        return {"error": str(e)}


def check_boot_type(client) -> dict[str, Any]:
    try:
        boot_type = client.getBootType()
        return {"boot_type": str(boot_type)}
    except Exception as e:
        return {"error": str(e)}


def check_config_info(client) -> dict[str, Any]:
    try:
        config_info = client.getConfigAppliedInfo()
        return {
            "last_config_applied_ms": getattr(config_info, "lastAppliedInMs", None),
        }
    except Exception as e:
        return {"error": str(e)}


def check_product_info(client) -> dict[str, Any]:
    try:
        product_info = client.getProductInfo()
        return {
            "product": getattr(product_info, "product", None),
            "serial": getattr(product_info, "serial", None),
            "version": getattr(product_info, "version", None),
        }
    except Exception as e:
        return {"error": str(e)}


def check_fabric_connectivity(client) -> dict[str, Any]:
    try:
        fabric = client.getFabricConnectivity()
        return {"endpoints": len(fabric)}
    except Exception as e:
        return {"error": str(e)}


def check_route_table(client) -> dict[str, Any]:
    try:
        routes = client.getRouteTableDetails()
        return {"route_count": len(routes)}
    except Exception as e:
        return {"error": str(e)}


def check_agent_status(client) -> dict[str, Any]:
    try:
        status = client.getHwAgentConnectionStatus()
        return {"agents": len(status)}
    except Exception as e:
        return {"error": str(e)}


def run_health_check(host: str, port: int = 5909) -> dict[str, Any]:
    print(f"\n{'=' * 60}")
    print(f"Health Check: {host}:{port}")
    print(f"{'=' * 60}")

    health: dict[str, Any] = {
        "host": host,
        "port": port,
        "status": "unknown",
        "checks": {},
    }

    try:
        client, transport = create_client(host, port)

        print("\n1. Port Status...")
        health["checks"]["port_status"] = check_port_status(client)
        ps = health["checks"]["port_status"]
        print(
            f"   Total: {ps['total_ports']}, Up: {ps['ports_up']}, Down: {ps['ports_down']}"
        )

        print("\n2. Switch State...")
        health["checks"]["switch_state"] = check_switch_state(client)
        print(f"   {health['checks']['switch_state']}")

        print("\n3. Boot Type...")
        health["checks"]["boot_type"] = check_boot_type(client)
        print(f"   {health['checks']['boot_type']}")

        print("\n4. LLDP Neighbors...")
        health["checks"]["lldp_neighbors"] = check_lldp_neighbors(client)
        lldp = health["checks"]["lldp_neighbors"]
        print(f"   {lldp}")

        print("\n5. ARP Table...")
        health["checks"]["arp_table"] = check_arp_table(client)
        print(f"   {health['checks']['arp_table']}")

        print("\n6. NDP Table...")
        health["checks"]["ndp_table"] = check_ndp_table(client)
        print(f"   {health['checks']['ndp_table']}")

        print("\n7. Route Table...")
        health["checks"]["route_table"] = check_route_table(client)
        print(f"   {health['checks']['route_table']}")

        print("\n8. Product Info...")
        health["checks"]["product_info"] = check_product_info(client)
        print(f"   {health['checks']['product_info']}")

        print("\n9. Config Info...")
        health["checks"]["config_info"] = check_config_info(client)
        print(f"   {health['checks']['config_info']}")

        print("\n10. Fabric Connectivity...")
        health["checks"]["fabric_connectivity"] = check_fabric_connectivity(client)
        print(f"   {health['checks']['fabric_connectivity']}")

        print("\n11. HW Agent Status...")
        health["checks"]["agent_status"] = check_agent_status(client)
        print(f"   {health['checks']['agent_status']}")

        health["status"] = "healthy"
        transport.close()

    except Exception as e:
        health["status"] = "error"
        health["error"] = str(e)
        print(f"\nERROR: {e}")

    return health


def print_summary(results: list[dict[str, Any]]):
    print(f"\n{'=' * 60}")
    print("HEALTH CHECK SUMMARY")
    print(f"{'=' * 60}")

    for r in results:
        status_icon = "PASS" if r["status"] == "healthy" else "FAIL"
        print(f"\n[{status_icon}] {r['host']}:{r['port']}")

        if r["status"] == "healthy":
            ps = r["checks"].get("port_status", {})
            print(
                f"  Ports: {ps.get('ports_up', 0)} up / {ps.get('ports_down', 0)} down"
            )

            lldp = r["checks"].get("lldp_neighbors", {})
            if "count" in lldp:
                print(f"  LLDP Neighbors: {lldp['count']}")

            rt = r["checks"].get("route_table", {})
            if "route_count" in rt:
                print(f"  Routes: {rt['route_count']}")


def main():
    parser = argparse.ArgumentParser(description="FBOSS System Health Validation")
    parser.add_argument(
        "--hosts",
        nargs="+",
        default=[
            "susw001.r147.c081.dkl6",
            "susw002.r147.c081.dkl6",
            "susw003.r147.c081.dkl6",
            "susw004.r147.c081.dkl6",
        ],
        help="Switch hostnames",
    )
    parser.add_argument("--port", type=int, default=5909, help="Thrift port")
    parser.add_argument("--json", action="store_true", help="Output as JSON")
    args = parser.parse_args()

    results = []
    for host_entry in args.hosts:
        if ":" in host_entry:
            host, port = host_entry.rsplit(":", 1)
            port = int(port)
        else:
            host = host_entry
            port = args.port
        result = run_health_check(host, port)
        results.append(result)

    if args.json:
        print(json.dumps(results, indent=2, default=str))
    else:
        print_summary(results)


if __name__ == "__main__":
    main()
