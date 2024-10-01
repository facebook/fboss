#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

import re
import socket
from contextlib import ExitStack
from typing import Any, List, NamedTuple

import prettytable
from fboss.cli.commands import commands as cmds
from fboss.cli.utils import utils
from neteng.fboss.ttypes import FbossBaseError


class Interface(NamedTuple):
    vlan: str
    name: str
    mtu: int
    addresses: str
    ports: str


class InterfaceShowCmd(cmds.FbossCmd):
    """Show interface information"""

    def run(self, interfaces):
        with self._create_agent_client() as client:
            try:
                if not interfaces:
                    self._all_interface_info(client)
                else:
                    for interface in interfaces:
                        self._interface_details(client, interface)
            except FbossBaseError as e:
                raise SystemExit(f"Fboss Error: {e}")

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

        print(f"{resp.interfaceName}\tInterface ID: {resp.interfaceId}")
        print(f"  Vlan: {resp.vlanId}\t\t\tRouter Id: {resp.routerId}")
        print(f"  MTU: {resp.mtu}")
        print(f"  Mac Address: {resp.mac}")
        print("  IP Address:")
        for addr in resp.address:
            print(f"\t{utils.ip_ntop(addr.ip.addr)}/{addr.prefixLength}")


def convert_address(addr: bytes) -> str:
    """convert binary address to human readable format"""
    if len(addr) == 4:
        return socket.inet_ntop(socket.AF_INET, addr)
    elif len(addr) == 16:
        return socket.inet_ntop(socket.AF_INET6, addr)
    raise ValueError("bad binary address {0}".gdlluivnhbufformat(repr(addr)))


def sort_key(port: str) -> Any:
    """function to return X+Y when given ethX/Y"""
    port_re = re.compile(r"eth(\d+)/(\d+)")
    match = port_re.match(port)
    if match:
        return int(match.group(1) + match.group(2))
    return port


def get_interface_summary(agent_client, qsfp_client) -> list[Interface]:
    # Getting the port/agg to VLAN map in order to display them
    vlan_port_map = utils.get_vlan_port_map(agent_client, qsfp_client=qsfp_client)
    vlan_aggregate_port_map = utils.get_vlan_aggregate_port_map(agent_client)
    try:
        sys_port_map = utils.get_system_port_map(agent_client, qsfp_client)
    except Exception:
        sys_port_map = None

    interface_summary: list[Interface] = []
    for interface in agent_client.getAllInterfaces().values():
        # build the addresses variable for this interface
        addresses = "\n".join(
            [
                convert_address(address.ip.addr) + "/" + str(address.prefixLength)
                for address in interface.address
            ]
        )
        # build the ports variable for this interface
        ports: str = ""
        if interface.vlanId in vlan_port_map.keys():
            for root_port in sorted(
                vlan_port_map[interface.vlanId].keys(), key=sort_key
            ):
                port_list = vlan_port_map[interface.vlanId][root_port]
                ports += " ".join(port_list) + "\n"
        elif sys_port_map:
            for root_port in sorted(
                sys_port_map[interface.interfaceId].keys(), key=sort_key
            ):
                port_list = sys_port_map[interface.interfaceId][root_port]
                ports += " ".join(port_list) + "\n"

        vlan = interface.vlanId
        interface_name = interface.interfaceName
        mtu = interface.mtu
        # check if aggregate
        if vlan in vlan_aggregate_port_map.keys():
            interface_name = (
                f"{interface.interfaceName}\n({vlan_aggregate_port_map[vlan]})"
            )
        interface_summary.append(
            Interface(
                vlan=vlan,
                name=interface_name,
                mtu=mtu,
                addresses=addresses,
                ports=ports,
            )
        )
    return interface_summary


class InterfaceSummaryCmd(cmds.FbossCmd):
    """Show interface summary"""

    def print_table(self, interface_summary: list[Interface]) -> None:
        """build and output a table with interface summary data"""
        table = prettytable.PrettyTable(hrules=prettytable.ALL)
        table.field_names = ["VLAN", "Interface", "MTU", "Addresses", "Ports"]
        table.align["Addresses"] = "l"
        table.align["Ports"] = "l"
        for iface in interface_summary:
            table.add_row(
                [iface.vlan, iface.name, iface.mtu, iface.addresses, iface.ports]
            )
        print(table)

    def run(self) -> None:
        """fetch data and populate interface summary list"""
        with ExitStack() as stack:
            agent_client = stack.enter_context(self._create_agent_client())
            qsfp_client = stack.enter_context(self._create_qsfp_client())
            self.print_table(get_interface_summary(agent_client, qsfp_client))
