#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

import socket

from fboss.cli.commands import commands as cmds
from fboss.cli.utils import utils


class AclTableCmd(cmds.FbossCmd):
    def getProtoString(self, proto):
        if proto == socket.IPPROTO_TCP:
            return "tcp"
        if proto == socket.IPPROTO_UDP:
            return "udp"
        return str(proto)

    def run(self):
        with self._create_agent_client() as client:
            resp = client.getAclTable()

        if not resp:
            print("No Acl Entries Found")
            return
        resp = sorted(resp, key=lambda x: (x.priority))
        for entry in resp:
            print("Acl: %s" % entry.name)
            print("   priority: %d" % entry.priority)
            if entry.srcIp.addr:
                print(
                    "   src ip: %s/%d"
                    % (utils.ip_ntop(entry.srcIp.addr), entry.srcIpPrefixLength)
                )
            if entry.dstIp.addr:
                print(
                    "   dst ip: %s/%d"
                    % (utils.ip_ntop(entry.dstIp.addr), entry.dstIpPrefixLength)
                )
            if entry.proto:
                print(
                    "   proto: %d(%s)" % (entry.proto, self.getProtoString(entry.proto))
                )
            if entry.srcPort:
                print("   src port: %d" % entry.srcPort)
            if entry.dstPort:
                print("   dst port: %d" % entry.dstPort)
            if entry.ipFrag:
                print("   ip fragment: %d" % entry.ipFrag)
            if entry.dscp:
                print("   dscp: %d" % entry.dscp)
            if entry.ipType:
                print("   ip type: %d" % entry.ipType)
            if entry.icmpType:
                print("   icmp type: %d" % entry.icmpType)
            if entry.icmpCode:
                print("   icmp code: %d" % entry.icmpCode)
            if entry.ttl:
                print("   icmp code: %d" % entry.ttl)
            if entry.l4SrcPort:
                print(
                    "   L4 src port: %d(%s)"
                    % (entry.l4SrcPort, socket.getservbyport(entry.l4SrcPort))
                )
            if entry.l4DstPort:
                print(
                    "   L4 dst port: %d(%s)"
                    % (entry.l4DstPort, socket.getservbyport(entry.l4DstPort))
                )
            if entry.dstMac:
                print("   dst mac: %s" % entry.dstMac)
            print("   action: %s" % entry.actionType)

            print()
