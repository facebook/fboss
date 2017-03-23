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
from fboss.cli.utils import utils


class L2TableCmd(cmds.FbossCmd):
    def run(self):
        self._client = self._create_agent_client()
        resp = self._client.getL2Table()
        port_map = self._client.getAllPortInfo()

        if not resp:
            print("No L2 Entries Found")
            return
        resp = sorted(resp, key=lambda x: (x.port, x.vlanID, x.mac))
        tmpl = "{:18} {:>4}  {}"

        print(tmpl.format("MAC Address", "Port", "VLAN"))
        for port_info in sorted(port_map.values(), key=utils.port_sort_fn):
            for entry in resp:
                port_data = port_info.portId
                if port_data == entry.port:
                    if port_info.name:
                        port_data = port_info.name
                    print(tmpl.format(entry.mac, port_data, entry.vlanID))
