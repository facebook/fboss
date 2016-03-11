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

from fboss.cli.commands import commands as cmds


class L2TableCmd(cmds.FbossCmd):
    def run(self):
        self._client = self._create_ctrl_client()
        resp = self._client.getL2Table()

        if not resp:
            print("No L2 Entries Found")
            return
        resp = sorted(resp, key=lambda x: (x.port, x.vlanID, x.mac))
        tmpl = "{:18} {:>4}  {}"

        print(tmpl.format("MAC Address", "Port", "VLAN"))
        for entry in resp:
            print(tmpl.format(entry.mac, entry.port, entry.vlanID))
