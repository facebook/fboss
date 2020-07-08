#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

from fboss.cli.commands import commands as cmds
from neteng.fboss.ctrl.ttypes import L2EntryType


class L2TableCmd(cmds.FbossCmd):
    def run(self):
        with self._create_agent_client() as client:
            resp = client.getL2Table()
            port_map = client.getAllPortInfo()
            agg_ports = client.getAggregatePortTable()

        if not resp:
            print("No L2 Entries Found")
            return
        resp = sorted(resp, key=lambda x: (x.port, x.vlanID, x.mac))

        tmpl = "{:18} {:>17}  {:<10} {:15} {}"
        print(tmpl.format("MAC Address", "Port/Trunk", "VLAN", "TYPE", "CLASSID"))

        for entry in resp:
            if entry.trunk:
                relevant = [
                    agg_port for agg_port in agg_ports if agg_port.key == entry.trunk
                ]
                if relevant and len(relevant) == 1:
                    port_data = relevant[0].name
                else:
                    port_data = f"{entry.trunk} (Trunk)"
            else:
                port_info = port_map.get(entry.port, None)
                if not port_info:
                    # Skip ports for which we could not lookup port_info
                    # This is typically the entries associated with CPU
                    # port, all of which just point to the MAC address
                    # we assigned to they configured vlans.
                    continue
                port_data = port_info.name if port_info.name else entry.port

            if (hasattr(entry, "l2EntryType")) and entry.l2EntryType is not None:
                if entry.l2EntryType == L2EntryType.L2_ENTRY_TYPE_PENDING:
                    entry_type = "Pending"
                elif entry.l2EntryType == L2EntryType.L2_ENTRY_TYPE_VALIDATED:
                    entry_type = "Validated"
                else:
                    entry_type = "Unknown"
            else:
                entry_type = "-"

            if (hasattr(entry, "classID")) and entry.classID is not None:
                class_id = entry.classID
            else:
                class_id = "-"

            print(tmpl.format(entry.mac, port_data, entry.vlanID, entry_type, class_id))
