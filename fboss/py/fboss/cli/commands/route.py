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
from thrift.Thrift import TApplicationException


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
            print("    %s" % utils.ip_ntop(nextHop.addr))
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

    def printIpRoute(self, addr, vrf):
        resp = self._client.getIpRoute(addr, vrf)
        print('Route to ' + addr.addr + ', Vrf: %d' % vrf)
        netAddr = utils.ip_ntop(resp.dest.ip.addr)
        prefix = resp.dest.prefixLength
        if netAddr and resp.nextHopAddrs:
            print('N/w: %s/%d' % (netAddr, prefix))
            for nexthops in resp.nextHopAddrs:
                print('\t\tvia: ' + utils.ip_ntop(nexthops.addr))
        else:
            print('No Route to destination')

    def run(self, ip, vrf):
        addr = Address(addr=ip, type=AddressType.V4)
        if not addr.addr:
            print('No ip address provided')
            return
        self._client = self._create_agent_client()
        try:
            self.printIpRouteDetails(addr, vrf)
        except TApplicationException:
            self.printIpRoute(addr, vrf)


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
            for nhop in entry.nextHopAddrs:
                print("\tvia %s" % utils.ip_ntop(nhop.addr))


class RouteTableDetailsCmd(cmds.FbossCmd):
    def run(self):
        self._client = self._create_agent_client()
        resp = self._client.getRouteTableDetails()
        if not resp:
            print("No Route Table Details Found")
            return
        for entry in resp:
            printRouteDetailEntry(entry)
