#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

import prettytable
import re
import socket
import sys

from typing import Any, Dict, List, NamedTuple, Optional

from fboss.cli.commands import commands as cmds
from fboss.cli.utils import utils
from libfb.py.decorators import retryable
from neteng.fboss.ttypes import FbossBaseError


class Interface(NamedTuple):
    vlan: str
    name: str
    mtu: int
    addresses: str
    ports: str


class InterfaceShowCmd(cmds.FbossCmd):
    ''' Show interface information '''
    def run(self, interfaces):
        with self._create_agent_client() as client:
            try:
                if not interfaces:
                    self._all_interface_info(client)
                else:
                    for interface in interfaces:
                        self._interface_details(client, interface)
            except FbossBaseError as e:
                raise SystemExit('Fboss Error: {}'.format(e))

    def _all_interface_info(self, client):
        resp = client.getInterfaceList()
        if not resp:
            print("No Interfaces Found")
            return
        for entry in resp:
            print(entry)

    def _interface_details(self, client, interface):
        resp = client.getInterfaceDetail(interface)
        if not resp:
            print("No interface details found for interface")
            return

        print("{}\tInterface ID: {}".format(
            resp.interfaceName, resp.interfaceId))
        print("  Vlan: {}\t\t\tRouter Id: {}".format(
            resp.vlanId, resp.routerId))
        print("  MTU: {}".format(resp.mtu))
        print("  Mac Address: {}".format(resp.mac))
        print("  IP Address:")
        for addr in resp.address:
            print("\t{}/{}".format(utils.ip_ntop(addr.ip.addr),
                                   addr.prefixLength))


class InterfaceSummaryCmd(cmds.FbossCmd):
    ''' Show interface summary '''

    def convert_address(self, addr: bytes) -> str:
        ''' convert binary address to human readable format '''
        if len(addr) == 4:
            return socket.inet_ntop(socket.AF_INET, addr)
        elif len(addr) == 16:
            return socket.inet_ntop(socket.AF_INET6, addr)
        raise ValueError("bad binary address {0}".format(repr(addr)))

    def get_port_summary(
        self,
        port_name: str,
        channels: List[int],
        qsfp_present: bool,
        fab_port: bool,
        enabled: bool,
        speed: int,
        up: bool,
    ) -> Optional[str]:
        ''' build the port summary output taking into account various state '''
        COLOR_RED, COLOR_GREEN, COLOR_RESET = utils.get_colors()
        # port has channels assigned with sfp present or fab port, is enabled/up
        if ((channels and qsfp_present) or fab_port) and enabled and up:
            return f'{COLOR_GREEN}{port_name}{COLOR_RESET} ({speed}G)'
        # port has channels assigned with sfp present or fab port, is enabled/down
        if ((channels and qsfp_present) or fab_port) and enabled and not up:
            return f'{COLOR_RED}{port_name}{COLOR_RESET} ({speed}G)'
        # port has channels assigned with no sfp present, is enabled/up
        if (channels and not qsfp_present) and enabled:
            return f'{port_name} ()'
        # port is of no interest (no channel assigned, no SFP, or disabled)
        return None

    @retryable(num_tries=3, sleep_time=0.1)
    def vlan_aggregate_port_map(self, client) -> Dict:
        ''' fetch aggregate port table and map vlan -> port channel name'''
        aggregate_port_table = client.getAggregatePortTable()
        vlan_aggregate_port_map: Dict = {}
        for aggregate_port in aggregate_port_table:
            agg_port_name = aggregate_port.name
            for member_port in aggregate_port.memberPorts:
                vlans = client.getPortInfo(member_port.memberPortID).vlans
                assert len(vlans) == 1
                vlan = vlans[0]
                vlan_aggregate_port_map[vlan] = agg_port_name
        return vlan_aggregate_port_map

    @retryable(num_tries=3, sleep_time=0.1)
    def get_vlan_port_map(self, agent_client, qsfp_client) -> Dict:
        ''' fetch port info and map vlan -> ports '''
        all_port_info_map = agent_client.getAllPortInfo()
        port_status_map = agent_client.getPortStatus()
        qsfp_info_map = utils.get_qsfp_info_map(
            qsfp_client, None, continue_on_error=True
        )
        vlan_port_map: Dict = {}
        for port in all_port_info_map.values():
            vlan_count = len(port.vlans)
            # unconfigured ports can be skipped
            if vlan_count == 0:
                continue
            # fboss ports currently only support a single vlan
            assert vlan_count == 1
            vlan = port.vlans[0]
            # root port is the parent physical port
            match = re.match(r"(.*)\/\d+$", port.name)
            if match:
                root_port = match.group(1)
            else:
                sys.exit(f"root_port for {port.name} could not be determined")
            port_status = port_status_map.get(port.portId)
            enabled = port_status.enabled
            up = port_status.up
            speed = int(port_status.speedMbps / 1000)
            # ports with transceiver data
            channels: List = []
            qsfp_id = None
            if port_status.transceiverIdx:
                channels = port_status.transceiverIdx.channels
                qsfp_id = port_status.transceiverIdx.transceiverId
            # galaxy fab ports have no transceiver
            fab_port = 'fab' in port.name
            qsfp_info = qsfp_info_map.get(qsfp_id)
            qsfp_present = qsfp_info.present if qsfp_info else False
            port_summary = self.get_port_summary(
                port.name, channels, qsfp_present, fab_port, enabled, speed, up
            )
            if not port_summary:
                continue
            if vlan in vlan_port_map.keys():
                if root_port in vlan_port_map[vlan].keys():
                    vlan_port_map[vlan][root_port].append(port_summary)
                else:
                    vlan_port_map[vlan][root_port] = [port_summary]
            else:
                vlan_port_map[vlan] = {root_port: [port_summary]}

        return vlan_port_map

    def print_table(self, interface_summary: List[Interface]) -> None:
        ''' build and output a table with interface summary data '''
        table = prettytable.PrettyTable(hrules=prettytable.ALL)
        table.field_names = ['VLAN', 'Interface', 'MTU', 'Addresses', 'Ports']
        table.align["Addresses"] = "l"
        table.align["Ports"] = "l"
        for iface in interface_summary:
            table.add_row([
                iface.vlan,
                iface.name,
                iface.mtu,
                iface.addresses,
                iface.ports,
            ])
        print(table)

    def sort_key(self, port: str) -> Any:
        ''' function to return X+Y when given ethX/Y '''
        port_re = re.compile(r'eth(\d+)/(\d+)')
        match = port_re.match(port)
        if match:
            return int(match.group(1) + match.group(2))
        return port

    def run(self) -> None:
        ''' fetch data and populate interface summary list '''
        with self._create_agent_client() as agent_client, \
                self._create_qsfp_client() as qsfp_client:
            vlan_port_map = self.get_vlan_port_map(agent_client,
                    qsfp_client)
            vlan_aggregate_port_map = self.vlan_aggregate_port_map(agent_client)
            interface_summary: List[Interface] = []
            for interface in agent_client.getAllInterfaces().values():
                # build the addresses variable for this interface
                addresses = '\n'.join([
                    self.convert_address(address.ip.addr) +
                    '/' + str(address.prefixLength)
                    for address in interface.address
                ])
                # build the ports variable for this interface
                ports: str = ''
                if interface.vlanId in vlan_port_map.keys():
                    for root_port in sorted(
                        vlan_port_map[interface.vlanId].keys(), key=self.sort_key,
                    ):
                        port_list = vlan_port_map[interface.vlanId][root_port]
                        ports += ' '.join(port_list) + '\n'
                vlan = interface.vlanId
                interface_name = interface.interfaceName
                mtu = interface.mtu
                # check if aggregate
                if vlan in vlan_aggregate_port_map.keys():
                    interface_name = (
                        f'{interface.interfaceName}\n({vlan_aggregate_port_map[vlan]})'
                    )
                interface_summary.append(Interface(
                    vlan=vlan,
                    name=interface_name,
                    mtu=mtu,
                    addresses=addresses,
                    ports=ports,
                ))
            self.print_table(interface_summary)
