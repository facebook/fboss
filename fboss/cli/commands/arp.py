#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#
# @lint-avoid-pyflakes2
# @lint-avoid-python-3-compatibility-imports

from fboss.cli import utils
from fboss.cli.commands import commands as cmds


class ArpTableCmd(cmds.FbossCmd):
    def run(self):
        self._client = self._create_ctrl_client()
        resp = self._client.getArpTable()

        if not resp:
            print("No ARP Entries Found")
            return

        tmpl = "{:" + str(16) + "} {:18} {:>4}  {}"
        print(tmpl.format("IP Address", "MAC Address", "Port", "VLAN"))
        for entry in resp:
            ip = utils.ip_ntop(entry.ip.addr)
            vlan_field = '{} ({})'.format(entry.vlanName, entry.vlanID)
            print(tmpl.format(ip, entry.mac, entry.port, vlan_field))
