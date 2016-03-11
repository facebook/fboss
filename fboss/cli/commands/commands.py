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
from fboss.thrift_clients import FbossAgentClient


# Parent Class for all commands
class FbossCmd(object):
    def __init__(self, cli_opts):
        ''' initialize; client will be created in subclasses with the specific
            client they need '''

        self._hostname = cli_opts.hostname
        self._port = cli_opts.port
        self._timeout = cli_opts.timeout
        self._client = None

    def _create_ctrl_client(self):
        args = [self._hostname, self._port]
        if self._timeout:
            args.append(self._timeout)

        return FbossAgentClient(*args)


# --- All generic commands below -- #

class NeighborFlushCmd(FbossCmd):
    def run(self, ip, vlan):
        bin_ip = utils.ip_to_binary(ip)
        vlan_id = vlan
        client = self._create_ctrl_client()
        num_entries = client.flushNeighborEntry(bin_ip, vlan_id)
        print('Flushed {} entries'.format(num_entries))
