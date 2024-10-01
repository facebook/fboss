#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

import binascii

from fboss.cli.commands import commands as cmds
from fboss.cli.utils import utils


class LldpCmd(cmds.FbossCmd):
    def _get_lldp_neighbors(self):
        with self._create_agent_client() as client:
            lldp_nbrs = client.getLldpNeighbors()

        return sorted(lldp_nbrs, key=self._port_sort_fn) if lldp_nbrs else None

    def run(self, lldp_port, verbosity):
        lldp_nbrs = self._get_lldp_neighbors()

        if not lldp_nbrs:
            print("No neighbors found")
            return

        headers = {
            "local_port": "Local Port",
            "local_vlan": "Local VLAN",
            "mac": "MAC",
            "chassis": "Chassis ID",
            "raw_chassis": "Raw Chassis ID",
            "chassis_id_type": "Chassis ID Type",
            "name": "Name",
            "system": "System Name",
            "port": "Port",
            "raw_port": "Raw Port ID",
            "port_id_type": "Port ID Type",
            "system_desc": "System Description",
            "port_desc": "Port Description",
            "ttl": "TTL",
            "ttl_left": "TTL Left",
        }
        max_widths = {key: len(value) + 1 for key, value in headers.items()}

        entries = []
        for neighbor in lldp_nbrs:
            if lldp_port and not neighbor.localPort == lldp_port:
                continue
            if not neighbor.systemName:
                # Skipping neighbors without name populated
                continue
            fields = self._get_fields(neighbor)
            for key, value in fields.items():
                if len(str(value)) > max_widths[key]:
                    max_widths[key] = len(value)
            entries.append(fields)

        if not verbosity:
            selected = ["local_port", "name", "port"]
            self._print_fields(selected, entries, headers, max_widths)
        elif verbosity == 1:
            selected = ["local_port", "chassis", "system", "port", "ttl", "ttl_left"]
            self._print_fields(selected, entries, headers, max_widths)
        else:
            self._print_verbose(entries, headers)

    def _port_sort_fn(self, neighbor):
        port_name = neighbor.localPortName
        return utils.port_name_sort_fn(port_name)

    def _print_fields(self, selected, fields, headers, max_widths):
        fmt = " ".join(f"{{{key}:<{max_widths[key]}}}" for key in selected)

        header = fmt.format(**headers)
        print(header)
        print("-" * len(header))

        for entry in fields:
            line = fmt.format(**entry)
            print(line)

    def _print_verbose(self, entries, headers):
        for entry in entries:
            print("Neighbor:")
            print("  Local Port: {}".format(entry["local_port"]))
            print("  Local VLAN: {}".format(entry["local_vlan"]))
            print("  Original TTL: {}".format(entry["ttl"]))
            print("  TTL Left: {}".format(entry["ttl_left"]))
            print("  Source MAC: {}".format(entry["mac"]))
            print("  Chassis ID Type: {}".format(entry["chassis_id_type"]))

            raw_chassis = entry["raw_chassis"]
            if self.is_printable(raw_chassis):
                print(f"  Chassis ID: {raw_chassis}")
            else:
                rc = binascii.hexlify(raw_chassis)
                print(f"  Raw Chassis ID: hex({rc})")
                print("  Chassis ID: {}".format(entry["chassis"]))

            raw_port = entry["raw_port"]
            if self.is_printable(raw_port):
                print(f"  Port ID: {raw_port}")
            else:
                rc = binascii.hexlify(raw_port)
                print(f"  Raw Port ID: hex({rc})")
                print("  Port ID: {}".format(entry["port"]))

            print("  System Name: {}".format(entry["system"]))

            desc = entry["system_desc"]
            if "\n" in desc:
                desc = "\n    ".join(desc.splitlines())
                print(f"  System Description:\n    {desc}")
            else:
                print(f"  System Description: {desc}")
            print("  Port Description: {}".format(entry["port_desc"]))
            print()

    def is_printable(self, value):
        try:
            value.decode("utf-8")
            return True
        except UnicodeDecodeError:
            return False

    def _get_fields(self, neighbor):
        fields = {}
        fields["local_port"] = neighbor.localPortName
        fields["local_vlan"] = neighbor.localVlan
        fields["mac"] = neighbor.srcMac
        fields["chassis"] = neighbor.printableChassisId
        fields["raw_chassis"] = neighbor.chassisId
        fields["chassis_id_type"] = neighbor.chassisIdType
        fields["system"] = neighbor.systemName
        fields["port"] = neighbor.printablePortId
        fields["raw_port"] = neighbor.portId
        fields["port_id_type"] = neighbor.portIdType
        if neighbor.systemName is None:
            fields["name"] = neighbor.printableChassisId
        else:
            fields["name"] = neighbor.systemName
        fields["system_desc"] = neighbor.systemDescription or ""
        fields["port_desc"] = neighbor.portDescription or ""
        fields["ttl"] = neighbor.originalTTL
        fields["ttl_left"] = neighbor.ttlSecondsLeft
        return fields
