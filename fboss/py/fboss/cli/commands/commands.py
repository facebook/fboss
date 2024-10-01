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

from fboss.cli.utils import utils
from fboss.py.fboss.thrift_clients import (
    PlainTextFbossAgentClientDontUseInFb as PlainTextFbossAgentClient,
    QsfpServiceClient,
)


class FlushType:
    arp, ndp = range(2)


# Parent Class for all commands
class FbossCmd:
    def __init__(self, cli_opts):
        """initialize; client will be created in subclasses with the specific
        client they need"""

        self._hostname = cli_opts.hostname
        self._port = cli_opts.port
        self._timeout = cli_opts.timeout
        self._client = None

    def _create_agent_client(self):
        args = [self._hostname, self._port]
        if self._timeout:
            args.append(self._timeout)

        return PlainTextFbossAgentClient(*args)

    def _create_qsfp_client(self):
        args = [self._hostname, self._port]
        if self._timeout:
            args.append(self._timeout)
        return QsfpServiceClient(*args)


# --- All generic commands below -- #


class NeighborFlushSubnetCmd(FbossCmd):
    def _flush_entry(self, ip, vlan):
        bin_ip = utils.ip_to_binary(ip)
        vlan_id = vlan
        with self._create_agent_client() as client:
            num_entries = client.flushNeighborEntry(bin_ip, vlan_id)
        print(f"Flushed {num_entries} entries")

    def run(self, flushType, network, vlan):
        if (
            isinstance(network, ipaddress.IPv6Network) and network.prefixlen == 128
        ) or (isinstance(network, ipaddress.IPv4Network) and network.prefixlen == 32):
            self._flush_entry(str(network.network_address), vlan)
            return

        with self._create_agent_client() as client:
            if flushType == FlushType.arp:
                table = client.getArpTable()
            elif flushType == FlushType.ndp:
                table = client.getNdpTable()
            else:
                print("Invaid flushType")
                exit(1)

            num_entries = 0
            for entry in table:
                if (
                    ipaddress.ip_address(utils.ip_ntop(entry.ip.addr))
                    in ipaddress.ip_network(network)
                ) and (vlan == 0 or vlan == entry.vlanID):
                    num_entries += client.flushNeighborEntry(entry.ip, entry.vlanID)

        print(f"Flushed {num_entries} entries")


class PrintNeighborTableCmd(FbossCmd):
    def __init__(self, cli_opts):
        super().__init__(cli_opts)
        self.nbr_table = None

    def run(self):
        with self._create_agent_client() as client:
            self.nbr_table = self._get_nbr_table(client)
            ports = client.getAllPortInfo()
        self._print_table(self.nbr_table, self.WIDTH, ports)

    def _print_table(self, entries, width, ports):
        tmpl = "{:" + str(width) + "} {:18} {:<10}  {:18} {!s:12} {:<10} {}"
        print(
            tmpl.format(
                "IP Address", "MAC Address", "Port", "VLAN", "State", "TTL", "CLASSID"
            )
        )

        for entry in sorted(entries, key=lambda e: e.ip.addr):
            port_identifier = entry.port
            if entry.port in ports and ports[entry.port].name:
                port_identifier = ports[entry.port].name
            ip = utils.ip_ntop(entry.ip.addr)
            vlan_field = f"{entry.vlanName} ({entry.vlanID})"
            ttl = f"{entry.ttl // 1000}s" if entry.ttl else "?"
            state = entry.state if entry.state else "NA"
            classID = entry.classID
            print(
                tmpl.format(
                    ip, entry.mac, port_identifier, vlan_field, state, ttl, classID
                )
            )


class VerbosityCmd(FbossCmd):
    def run(self, verbosity):
        with self._create_agent_client() as client:
            client.setOption("v", verbosity)
