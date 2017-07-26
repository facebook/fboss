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

from facebook.network.Address.ttypes import Address, AddressType
from fboss.cli.utils import utils
from fboss.cli.commands import commands as cmds
from neteng.fboss.ctrl.ttypes import IpPrefix
from neteng.fboss.ctrl.ttypes import UnicastRoute
from facebook.network.Address.ttypes import BinaryAddress


def nexthop_to_str(nh):
    ip_str = utils.ip_ntop(nh.addr)
    if not nh.ifName:
        return ip_str
    return "{}%{}".format(ip_str, nh.ifName)


def printRouteDetailEntry(entry):
    suffix = ""
    if (entry.isConnected):
        suffix += " (connected)"
    print("Network Address: %s/%d %s" %
          (utils.ip_ntop(entry.dest.ip.addr), entry.dest.prefixLength,
           suffix))
    for clAndNxthops in entry.nextHopMulti:
        print("  Nexthops from client %d" % clAndNxthops.clientId)
        for nextHop in clAndNxthops.nextHopAddrs:
            print("    %s" % nexthop_to_str(nextHop))
    print("  Action: %s" % entry.action)
    if len(entry.fwdInfo) > 0:
        print("  Forwarding via:")
        for ifAndIp in entry.fwdInfo:
            print("    (i/f %d) %s" % (ifAndIp.interfaceID,
                                       utils.ip_ntop(ifAndIp.ip.addr)))
    else:
        print("  No Forwarding Info")
    print("  Admin Distance: %s" % entry.adminDistance)
    print()


def parse_prefix(prefix):
    network = ipaddress.ip_network(prefix)
    return IpPrefix(ip=BinaryAddress(addr=network.network_address.packed),
                    prefixLength=network.prefixlen)


def parse_nexthops(nexthops):
    bin_addresses = []
    for nh_iface in nexthops:
        iface, addr = None, None
        # Nexthop may or may not be link-local. Handle it here well
        if '@' in nh_iface:
            addr, iface = nh_iface.split('@')
        elif '%' in nh_iface:
            addr, iface = nh_iface.split('%')
        else:
            addr = nh_iface
        bin_addresses.append(BinaryAddress(
            addr=ipaddress.ip_address(addr).packed,
            ifName=iface)
        )
    return bin_addresses


class RouteAddCmd(cmds.FbossCmd):
    def run(self, client_id, admin_distance, prefix, nexthops):
        prefix = parse_prefix(prefix)
        nexthops = parse_nexthops(nexthops)
        with self._create_agent_client() as client:
            client.addUnicastRoutes(
                client_id, [UnicastRoute(dest=prefix, nextHopAddrs=nexthops,
                                         adminDistance=admin_distance)]
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
    def printIpRouteDetails(self, client, addr, vrf):
        resp = client.getIpRouteDetails(addr, vrf)
        if not resp.nextHopMulti:
            print('No route to ' + addr.addr + ', Vrf: %d' % vrf)
            return
        print('Route to ' + addr.addr + ', Vrf: %d' % vrf)
        printRouteDetailEntry(resp)

    def run(self, ip, vrf):
        addr = Address(addr=ip, type=AddressType.V4)
        if not addr.addr:
            print('No ip address provided')
            return
        with self._create_agent_client() as client:
            self.printIpRouteDetails(client, addr, vrf)


class RouteTableCmd(cmds.FbossCmd):
    def run(self, client_id, ipv4, ipv6):
        with self._create_agent_client() as client:
            if client_id is None:
                resp = client.getRouteTable()
            else:
                resp = client.getRouteTableByClient(client_id)
            if not resp:
                print("No Route Table Entries Found")
                return
            for entry in resp:
                if ipv6 and not ipv4 and len(entry.dest.ip.addr) == 4:
                    continue
                if ipv4 and not ipv6 and len(entry.dest.ip.addr) == 16:
                    continue
                print("Network Address: %s/%d" %
                                    (utils.ip_ntop(entry.dest.ip.addr),
                                                entry.dest.prefixLength))
                # Need to check the nextHopAddresses
                for nextHop in entry.nextHopAddrs:
                    print("\tvia %s" % nexthop_to_str(nextHop))


class RouteTableDetailsCmd(cmds.FbossCmd):
    def run(self, ipv4, ipv6):
        with self._create_agent_client() as client:
            resp = client.getRouteTableDetails()
            if not resp:
                print("No Route Table Details Found")
                return
            for entry in resp:
                if ipv6 and not ipv4 and len(entry.dest.ip.addr) == 4:
                    continue
                if ipv4 and not ipv6 and len(entry.dest.ip.addr) == 16:
                    continue
                printRouteDetailEntry(entry)
