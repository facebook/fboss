#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

import ipaddress
import typing as t
from contextlib import ExitStack

from facebook.network.Address.ttypes import Address, AddressType, BinaryAddress
from fboss.cli.commands import commands as cmds
from fboss.cli.utils import utils
from neteng.fboss.ctrl.ttypes import IpPrefix, NextHopThrift, UnicastRoute


def printRouteDetailEntry(entry, vlan_aggregate_port_map, vlan_port_map):
    suffix = ""
    if entry.isConnected:
        suffix += " (connected)"
    print(
        "Network Address: %s/%d %s"
        % (utils.ip_ntop(entry.dest.ip.addr), entry.dest.prefixLength, suffix)
    )
    for clAndNxthops in entry.nextHopMulti:
        print("  Nexthops from client %d" % clAndNxthops.clientId)
        if clAndNxthops.nextHopAddrs:
            for address in clAndNxthops.nextHopAddrs:
                print("    %s" % utils.nexthop_to_str(NextHopThrift(address=address)))
        elif clAndNxthops.nextHops:
            for nextHop in clAndNxthops.nextHops:
                print("    %s" % utils.nexthop_to_str(nextHop))
    print("  Action: %s" % entry.action)
    if entry.nextHops and len(entry.nextHops) > 0:
        print("  Forwarding via:")
        for nextHop in entry.nextHops:
            print(
                "    {}".format(
                    utils.nexthop_to_str(
                        nextHop, vlan_aggregate_port_map, vlan_port_map
                    )
                )
            )
    elif len(entry.fwdInfo) > 0:
        print("  Forwarding via:")
        for ifAndIp in entry.fwdInfo:
            print(
                "    (i/f %d) %s"
                % (ifAndIp.interfaceID, utils.ip_ntop(ifAndIp.ip.addr))
            )
    else:
        print("  No Forwarding Info")
    print("  Admin Distance: %s" % entry.adminDistance)
    print()


def parse_prefix(prefix):
    network = ipaddress.ip_network(prefix)
    return IpPrefix(
        ip=BinaryAddress(addr=network.network_address.packed),
        prefixLength=network.prefixlen,
    )


def is_ucmp_active(next_hops: t.Iterator[NextHopThrift]) -> bool:
    """
    Whether or not UCMP is considered active.

    UCMP is considered inactive when all weight are the same
    for a given set of next hops, and active when they differ.
    """

    # Let's avoid crashing the CLI when next_hops is blank ;)
    if not next_hops:
        return False

    return not all(next_hops[0].weight == nh.weight for nh in next_hops)


def parse_nexthops(nexthops):
    nhts = []
    for nh in nexthops:
        iface = None
        weight = 0
        # Nexthop may have weight.
        if "x" in nh:
            addr_iface, _, weight = nh.rpartition("x")
            weight = int(weight)
        else:
            addr_iface = nh
        # Nexthop may or may not be link-local. Handle it here well
        if "@" in addr_iface:
            addr, _, iface = addr_iface.rpartition("@")
        elif "%" in addr_iface:
            addr, _, iface = addr_iface.rpartition("%")
        else:
            addr = addr_iface
        binaddr = BinaryAddress(addr=ipaddress.ip_address(addr).packed, ifName=iface)
        nhts.append(NextHopThrift(address=binaddr, weight=weight))
    return nhts


class RouteAddCmd(cmds.FbossCmd):
    def run(self, client_id, admin_distance, prefix, nexthops):
        prefix = parse_prefix(prefix)
        nexthops = parse_nexthops(nexthops)
        with self._create_agent_client() as client:
            client.addUnicastRoutes(
                client_id,
                [
                    UnicastRoute(
                        dest=prefix, nextHops=nexthops, adminDistance=admin_distance
                    )
                ],
            )


class RouteDelCmd(cmds.FbossCmd):
    def run(self, client_id, prefix):
        prefix = parse_prefix(prefix)
        with self._create_agent_client() as client:
            client.deleteUnicastRoutes(client_id, [prefix])


class RouteFlushCmd(cmds.FbossCmd):
    def run(self, client_id):
        with self._create_agent_client() as client:
            client.syncFib(client_id, [])


class RouteIpCmd(cmds.FbossCmd):
    def printIpRouteDetails(
        self, client, addr, vrf, vlan_aggregate_port_map, vlan_port_map
    ):
        resp = client.getIpRouteDetails(addr, vrf)
        if not resp.nextHopMulti:
            print("No route to " + addr.addr + ", Vrf: %d" % vrf)
            return
        print("Route to " + addr.addr + ", Vrf: %d" % vrf)
        printRouteDetailEntry(resp, vlan_aggregate_port_map, vlan_port_map)

    def run(self, ip, vrf):
        addr = Address(addr=ip, type=AddressType.V4)
        if not addr.addr:
            print("No ip address provided")
            return
        with ExitStack() as stack:
            client = stack.enter_context(self._create_agent_client())
            qsfp_client = stack.enter_context(self._create_qsfp_client())
            # Getting the port/agg to VLAN map in order to display them
            vlan_port_map = utils.get_vlan_port_map(
                client, qsfp_client, colors=False, details=False
            )
            vlan_aggregate_port_map = utils.get_vlan_aggregate_port_map(client)
            self.printIpRouteDetails(
                client, addr, vrf, vlan_aggregate_port_map, vlan_port_map
            )


class RouteTableCmd(cmds.FbossCmd):
    def run(self, client_id, ipv4, ipv6, prefixes: t.List[str]):
        with ExitStack() as stack:
            agent_client = stack.enter_context(self._create_agent_client())
            qsfp_client = stack.enter_context(self._create_qsfp_client())
            if client_id is None:
                resp = agent_client.getRouteTable()
            else:
                resp = agent_client.getRouteTableByClient(client_id)
            if not resp:
                print("No Route Table Entries Found")
                return

            # Getting the port/agg to VLAN map in order to display them
            vlan_port_map = utils.get_vlan_port_map(
                agent_client, qsfp_client, colors=False, details=False
            )
            vlan_aggregate_port_map = utils.get_vlan_aggregate_port_map(agent_client)

            for entry in resp:
                prefix_str = (
                    f"{utils.ip_ntop(entry.dest.ip.addr)}/{entry.dest.prefixLength}"
                )
                ucmp_active = " (UCMP Active)" if is_ucmp_active(entry.nextHops) else ""

                # Apply filters
                if ipv6 and not ipv4 and len(entry.dest.ip.addr) == 4:
                    continue
                if ipv4 and not ipv6 and len(entry.dest.ip.addr) == 16:
                    continue
                if prefixes and prefix_str not in prefixes:
                    continue

                # Print header
                print(f"Network Address: {prefix_str}{ucmp_active}")

                # Need to check the nextHopAddresses
                if entry.nextHops:
                    for nextHop in entry.nextHops:
                        print(
                            "\tvia %s"
                            % (
                                utils.nexthop_to_str(
                                    nextHop,
                                    vlan_aggregate_port_map=vlan_aggregate_port_map,
                                    vlan_port_map=vlan_port_map,
                                    ucmp_active=ucmp_active,
                                )
                            )
                        )
                else:
                    for address in entry.nextHopAddrs:
                        print(
                            "\tvia %s"
                            % utils.nexthop_to_str(NextHopThrift(address=address))
                        )


class RouteTableSummaryCmd(cmds.FbossCmd):
    """Print rough statistics of various route types
    Useful for understanding HW table capacity/usage
    """

    def run(self):
        with self._create_agent_client() as agent_client:
            resp = agent_client.getRouteTable()
        n_v4_routes = 0
        n_v6_small = 0
        n_v6_big = 0
        for entry in resp:
            if len(entry.dest.ip.addr) == 4:
                n_v4_routes += 1
            elif len(entry.dest.ip.addr) == 16:
                # break ipv6 up into <64 and >64  as it affect ASIC mem usage
                if entry.dest.prefixLength <= 64:
                    n_v6_small += 1
                else:
                    n_v6_big += 1
        # this is a very rough calculation and because of how the
        # hashing and fragmentation works should be taken with a grain of
        # salt rather than a fraction of available space
        hw_entries_used = n_v4_routes + 2 * n_v6_small + 4 * n_v6_big
        n_v6 = n_v6_big + n_v6_small
        print(
            f"""Route Table Summary:

{n_v4_routes:-10} v4 routes
{n_v6_small:-10} v6 routes (/64 or smaller)
{n_v6_big:-10} v6 routes (bigger than /64)
{n_v6:-10} v6 routes (total)
{hw_entries_used:-10} approximate hw entries used
"""
        )


class RouteTableDetailsCmd(cmds.FbossCmd):
    def run(self, ipv4, ipv6, prefixes: t.List[str]):
        with ExitStack() as stack:
            client = stack.enter_context(self._create_agent_client())
            qsfp_client = stack.enter_context(self._create_qsfp_client())
            vlan_port_map = utils.get_vlan_port_map(
                client, qsfp_client, colors=False, details=False
            )
            vlan_aggregate_port_map = utils.get_vlan_aggregate_port_map(client)
            resp = client.getRouteTableDetails()
            if not resp:
                print("No Route Table Details Found")
                return
            for entry in resp:
                prefix_str = (
                    f"{utils.ip_ntop(entry.dest.ip.addr)}/{entry.dest.prefixLength}"
                )

                # Apply filter
                if ipv6 and not ipv4 and len(entry.dest.ip.addr) == 4:
                    continue
                if ipv4 and not ipv6 and len(entry.dest.ip.addr) == 16:
                    continue
                if prefixes and prefix_str not in prefixes:
                    continue

                # Print route details
                printRouteDetailEntry(entry, vlan_aggregate_port_map, vlan_port_map)
