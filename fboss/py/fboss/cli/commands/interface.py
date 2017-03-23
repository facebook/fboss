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

from neteng.fboss.ttypes import FbossBaseError

class InterfaceCmd(cmds.FbossCmd):
    ''' Show interface information '''
    def run(self, interfaces):
        try:
            self._client = self._create_agent_client()
            if not interfaces:
                self._all_interface_info()
            else:
                for interface in interfaces:
                    self._interface_details(interface)
        except FbossBaseError as e:
            raise SystemExit('Fboss Error: {}'.format(e))

    def _all_interface_info(self):
        resp = self._client.getInterfaceList()
        if not resp:
            print("No Interfaces Found")
            return
        for entry in resp:
            print(entry)

    def _interface_details(self, interface):
        resp = self._client.getInterfaceDetail(interface)
        if not resp:
            print("No interface details found for interface")
            return

        print("%s\tInterface ID: %d" %
                            (resp.interfaceName, resp.interfaceId))
        print("  Vlan: %d\t\t\tRouter Id: %d" % (resp.vlanId, resp.routerId))
        print("  Mac Address: %s" % resp.mac)
        print("  IP Address:")
        for addr in resp.address:
            print("\t%s/%d" % (utils.ip_ntop(addr.ip.addr), addr.prefixLength))
