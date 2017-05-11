#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

from facebook.network.Address.ttypes import Address, AddressType
from fboss.cli.utils import utils
from fboss.cli.commands import commands as cmds


def nexthop_to_str(nh):
    ip_str = utils.ip_ntop(nh.addr)
    if not nh.ifName:
        return ip_str
    return "{}@{}".format(ip_str, nh.ifName)


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
    print()


class RouteIpCmd(cmds.FbossCmd):
    def printIpRouteDetails(self, addr, vrf):
        resp = self._client.getIpRouteDetails(addr, vrf)
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
        self._client = self._create_agent_client()
        self.printIpRouteDetails(addr, vrf)


class RouteTableCmd(cmds.FbossCmd):
    def run(self, client_id):
        self._client = self._create_agent_client()
        if client_id is None:
            resp = self._client.getRouteTable()
        else:
            resp = self._client.getRouteTableByClient(client_id)
        if not resp:
            print("No Route Table Entries Found")
            return
        for entry in resp:
            print("Network Address: %s/%d" %
                                (utils.ip_ntop(entry.dest.ip.addr),
                                            entry.dest.prefixLength))
            # Need to check the nextHopAddresses
            for nextHop in entry.nextHopAddrs:
                print("\tvia %s" % nexthop_to_str(nextHop))


class RouteTableDetailsCmd(cmds.FbossCmd):
    def run(self):
        self._client = self._create_agent_client()
        resp = self._client.getRouteTableDetails()
        if not resp:
            print("No Route Table Details Found")
            return
        for entry in resp:
            printRouteDetailEntry(entry)
