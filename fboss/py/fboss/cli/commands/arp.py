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


class ArpTableCmd(cmds.PrintNeighborTableCmd):
    WIDTH = 16

    def _get_nbr_table(self, client):
        return client.getArpTable()
