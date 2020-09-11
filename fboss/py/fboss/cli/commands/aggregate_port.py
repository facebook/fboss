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

from fboss.cli.commands import commands as cmds


AGG_PORT_NUM_RE = re.compile(r".*?(?P<port_num>[0-9]+)$")


def get_port_num(port: str) -> str:

    port_num_match = AGG_PORT_NUM_RE.match(port)
    if port_num_match:
        return port_num_match.group("port_num")

    return ""


def _should_print_agg_port_info(target_port_num: str, agg_port) -> bool:

    # If there's no target set we print the info for all ports
    if not target_port_num:
        return True

    if agg_port.key == int(target_port_num):
        return True

    return False


def _print_agg_port_sum(agg_port) -> None:
    active_members_count = _get_active_member_count(agg_port.memberPorts)

    print(f"\nPort name: {agg_port.name}")
    print(f"Description: {agg_port.description}")
    print(
        f"Active members/Configured members/Min members: "
        f"{active_members_count}/"
        f"{len(agg_port.memberPorts)}/"
        f"{agg_port.minimumLinkCount}"
    )


def _get_active_member_count(subports) -> int:
    active_members = [subport for subport in subports if subport.isForwarding]
    return len(active_members)


def _print_subport_info(name, subport) -> None:
    rate = "Fast" if subport.rate == 1 else "Slow"
    print(
        f"\tMember: {name}, "
        f"id: {subport.memberPortID}, "
        f"Up: {subport.isForwarding}, "
        f"Rate: {rate}"
    )


class AggregatePortCmd(cmds.FbossCmd):
    def legacy_format(self, response):
        resp = sorted(response, key=lambda x: x.aggregatePortId)

        for entry in resp:
            subports = entry.subports
            subports = sorted(subports, key=lambda x: x.id)
            print(
                "AggregatePortID: {} Num Ports: {}".format(
                    entry.aggregatePortId, len(subports)
                )
            )
            for subport in subports:
                print(
                    "\tSubport ID: {} Enabled: {}".format(
                        subport.id, subport.isForwardingEnabled
                    )
                )

    def run(self, port):

        port_num = get_port_num(port)
        if port and not port_num:
            print(f"Invalid aggregate port name: {port}")
            return

        with self._create_agent_client() as client:
            agg_port_table = client.getAggregatePortTable()

            port_details = client.getAllPortInfo()

        if not agg_port_table:
            print("No Aggregate Port Entries Found")
            return

        if not hasattr(agg_port_table[0], "key"):
            self.legacy_format(agg_port_table)
        else:
            agg_port_table = sorted(agg_port_table, key=lambda x: x.key)

            for entry in agg_port_table:
                if not _should_print_agg_port_info(port_num, entry):
                    continue

                _print_agg_port_sum(entry)
                for subport in sorted(entry.memberPorts, key=lambda x: x.memberPortID):
                    _print_subport_info(
                        port_details[subport.memberPortID].name, subport
                    )
