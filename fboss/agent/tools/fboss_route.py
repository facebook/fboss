#!/usr/bin/env python3
# Copyright (C) 2004-present Facebook. All Rights Reserved

import contextlib
import socket
from argparse import ArgumentParser

import ipaddr
from facebook.network.Address.ttypes import BinaryAddress
from neteng.fboss.ctrl import FbossCtrl
from neteng.fboss.ctrl.ttypes import IpPrefix, UnicastRoute
from neteng.fboss.qsfp import QsfpService
from thrift.protocol import TBinaryProtocol
from thrift.transport import TSocket


"""Add, change, or delete a route on FBOSS controller
"""


DEFAULT_CLIENTID = 1


def parse_prefix(args):
    network = ipaddr.IPNetwork(args.prefix)
    return IpPrefix(
        ip=BinaryAddress(addr=network.ip.packed), prefixLength=network.prefixlen
    )


def parse_nexthops(args):
    return [BinaryAddress(addr=ipaddr.IPAddress(nh).packed) for nh in args.nexthop]


def flush_routes(args):
    with get_client(args) as client:
        client.syncFib(args.client, [])


def add_route(args):
    prefix = parse_prefix(args)
    nexthops = parse_nexthops(args)
    with get_client(args) as client:
        client.addUnicastRoutes(
            args.client, [UnicastRoute(dest=prefix, nextHopAddrs=nexthops)]
        )


def del_route(args):
    prefix = parse_prefix(args)
    with get_client(args) as client:
        client.deleteUnicastRoutes(args.client, [prefix])


def list_intf(args):
    with get_client(args) as client:
        # for intf in client.getInterfaceList():
        for (
            idx,
            intf,
        ) in client.getAllInterfaces().items():  # noqa: B301 T25377293 Grandfathered in
            print("L3 Interface %d: %s" % (idx, format_interface(intf)))


def format_ip(ip):
    family = socket.AF_INET if len(ip.addr) == 4 else socket.AF_INET6
    return socket.inet_ntop(family, ip.addr)


def format_route(route):
    next_hops = ", ".join(format_ip(ip) for ip in route.nextHopAddrs)
    return "{} --> {}".format(format_prefix(route.dest), next_hops)


def format_prefix(prefix):
    return "%s/%d" % (format_ip(prefix.ip), prefix.prefixLength)


def format_interface(intf):
    return "{} ({})".format(", ".join(format_prefix(i) for i in intf.address), intf.mac)


def format_arp(arp):
    return "{} -> {}".format(format_ip(arp.ip), arp.mac)


def list_routes(args):
    with get_client(args) as client:
        for route in client.getRouteTable():
            print("Route %s" % format_route(route))


def list_optics(args):
    with get_qsfp_client(args) as client:
        info = client.getTransceiverInfo()
        for key, val in info.items():  # noqa: B301 T25377293 Grandfathered in
            print("Optic %d: %s" % (key, str(val)))


def list_ports(args):
    details = args.details
    with get_client(args) as client:
        for idx, intf in client.getPortStatus(
            list(range(1, 65))
        ).items():  # noqa: B301 T25377293 Grandfathered in
            stats = ""
            if details:
                stats = " (%s)" % client.getPortStats(idx)
            print(
                "Port %d: [enabled=%s, up=%s, present=%s]%s"
                % (idx, intf.enabled, intf.up, intf.present, stats)
            )


def list_arps(args):
    with get_client(args) as client:
        for arp in client.getArpTable():
            print("Arp: %s" % (format_arp(arp)))


def list_ndps(args):
    with get_client(args) as client:
        for ndp in client.getNdpTable():
            print("NDP: %s" % (format_arp(ndp)))


def list_vlans(args):
    with get_client(args) as client:
        # for intf in client.getInterfaceList():
        vlans = {}
        for _idx, intf in client.getAllInterfaces().items():
            vlans[intf.vlanId] = True
        for vlan in vlans:
            print("===== Vlan %d ==== " % vlan)
            for address in client.getVlanAddresses(vlan):
                print(address.addr)


def enable_port(args):
    port = args.en_port
    with get_client(args) as client:
        portnum = int(port)
        client.setPortState(portnum, True)
        print("Port %d enabled" % portnum)


def disable_port(args):
    port = args.dis_port
    with get_client(args) as client:
        portnum = int(port)
        client.setPortState(portnum, False)
        print("Port %d disabled" % portnum)


@contextlib.contextmanager
def get_client(args, timeout=5.0):
    sock = TSocket.TSocket(args.host, args.port)
    sock.setTimeout(timeout * 1000)  # thrift timeout is in ms
    protocol = TBinaryProtocol.TBinaryProtocol(sock)
    transport = protocol.trans
    transport.open()
    client = FbossCtrl.Client(protocol)
    yield client
    transport.close()


@contextlib.contextmanager
def get_qsfp_client(args, timeout=5.0):
    sock = TSocket.TSocket(args.host, args.port)
    sock.setTimeout(timeout * 1000)  # thrift timeout is in ms
    protocol = TBinaryProtocol.TBinaryProtocol(sock)
    transport = protocol.trans
    transport.open()
    client = QsfpService.Client(protocol)
    yield client
    transport.close()


def main() -> None:
    ap = ArgumentParser()
    ap.add_argument(
        "--port", "-p", type=int, default=5909, help="the controller thrift port"
    )
    ap.add_argument(
        "--client",
        "-c",
        type=int,
        default=DEFAULT_CLIENTID,
        help="the client ID used to manipulate the routes",
    )
    ap.add_argument("--host", help="the controller hostname", default="localhost")
    subparsers = ap.add_subparsers()

    flush_parser = subparsers.add_parser(
        "flush", help="flush all existing non-interface routes"
    )
    flush_parser.set_defaults(func=flush_routes)

    add_parser = subparsers.add_parser(
        "add", help="add a new route or change an existing route"
    )
    add_parser.set_defaults(func=add_route)
    add_parser.add_argument(
        "prefix", help='the route prefix, i.e. "1.1.1.0/24" or "2001::0/64"'
    )
    add_parser.add_argument(
        "nexthop",
        nargs="+",
        help='the nexthops of the route, i.e "10.1.1.1" or "2002::1"',
    )

    del_parser = subparsers.add_parser("delete", help="delete an existing route")
    del_parser.set_defaults(func=del_route)
    del_parser.add_argument(
        "prefix", help='The route prefix, i.e. "1.1.1.0/24" or "2001::0/64"'
    )

    list_parser = subparsers.add_parser("list_intf", help="list switch interfaces")
    list_parser.set_defaults(func=list_intf)
    list_parser.add_argument(
        "--details",
        action="store_true",
        help="List all information about the interface",
        default=False,
    )

    list_route_parser = subparsers.add_parser("list_routes", help="list switch routes")
    list_route_parser.set_defaults(func=list_routes)
    list_route_parser.add_argument(
        "--details",
        action="store_true",
        help="List all information about the routes",
        default=False,
    )

    list_optic_parser = subparsers.add_parser("list_optics", help="list switch optics")
    list_optic_parser.set_defaults(func=list_optics)
    list_optic_parser.add_argument(
        "--details",
        action="store_true",
        help="List all information about the optics",
        default=False,
    )

    list_port_parser = subparsers.add_parser("list_ports", help="list switch ports")
    list_port_parser.set_defaults(func=list_ports)
    list_port_parser.add_argument(
        "--details",
        action="store_true",
        help="List all information about the ports",
        default=False,
    )

    list_vlan_parser = subparsers.add_parser("list_vlans", help="list switch vlans")
    list_vlan_parser.set_defaults(func=list_vlans)
    list_vlan_parser.add_argument(
        "--details",
        action="store_true",
        help="List all information about the vlans",
        default=False,
    )

    list_arp_parser = subparsers.add_parser("list_arps", help="list switch arps")
    list_arp_parser.set_defaults(func=list_arps)
    list_arp_parser.add_argument(
        "--details",
        action="store_true",
        help="List all information about the arps",
        default=False,
    )

    list_ndp_parser = subparsers.add_parser("list_ndps", help="list switch ndps")
    list_ndp_parser.set_defaults(func=list_ndps)
    list_ndp_parser.add_argument(
        "--details",
        action="store_true",
        help="List all information about the ndps",
        default=False,
    )

    enable_port_parser = subparsers.add_parser("enable_port", help="Enable port")
    enable_port_parser.set_defaults(func=enable_port)
    enable_port_parser.add_argument("en_port", help="Port to enable")

    disable_port_parser = subparsers.add_parser("disable_port", help="Enable port")
    disable_port_parser.set_defaults(func=disable_port)
    disable_port_parser.add_argument("dis_port", action="store", help="Port to disable")

    args = ap.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
