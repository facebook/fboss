#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

from fboss.cli.utils import utils
from fboss.thrift_clients import FbossAgentClient


# Parent Class for all commands
class FbossCmd(object):
    def __init__(self, cli_opts):
        ''' initialize; client will be created in subclasses with the specific
            client they need '''

        self._hostname = cli_opts.hostname
        self._port = cli_opts.port
        self._timeout = cli_opts.timeout
        self._client = None

    def _create_ctrl_client(self):
        args = [self._hostname, self._port]
        if self._timeout:
            args.append(self._timeout)

        return FbossAgentClient(*args)


# --- All generic commands below -- #

class NeighborFlushCmd(FbossCmd):
    def run(self, ip, vlan):
        bin_ip = utils.ip_to_binary(ip)
        vlan_id = vlan
        client = self._create_ctrl_client()
        num_entries = client.flushNeighborEntry(bin_ip, vlan_id)
        print('Flushed {} entries'.format(num_entries))

class PrintNeighborTableCmd(FbossCmd):
    def print_table(self, entries, name, width, client=None):
        if client is None:
            client = self._create_ctrl_client()
        tmpl = "{:" + str(width) + "} {:18} {:<10}  {:18} {!s:12} {}"
        print(tmpl.format(
            "IP Address", "MAC Address", "Port", "VLAN", "State", "TTL"))
        ports = client.getAllPortInfo()

        for entry in sorted(entries, key=lambda e: e.ip.addr):
            port_identifier = entry.port
            if entry.port in ports and ports[entry.port].name:
                port_identifier = ports[entry.port].name
            ip = utils.ip_ntop(entry.ip.addr)
            vlan_field = '{} ({})'.format(entry.vlanName, entry.vlanID)
            ttl = '{}s'.format(entry.ttl // 1000) if entry.ttl else '?'
            state = entry.state if entry.state else 'NA'
            print(tmpl.format(ip, entry.mac, port_identifier, vlan_field,
                              state, ttl))
