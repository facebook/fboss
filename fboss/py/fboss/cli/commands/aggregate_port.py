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


class AggregatePortCmd(cmds.FbossCmd):
    def run(self):
        self._client = self._create_agent_client()
        resp = self._client.getAggregatePortTable()

        if not resp:
            print("No Aggregate Port Entries Found")
            return
        resp = sorted(resp, key=lambda x: x.aggregatePortId)

        for entry in resp:
            subports = entry.subports
            subports = sorted(subports, key=lambda x: x.id)
            print("AggregatePortID: {} Num Ports: {}".format(
                entry.aggregatePortId,
                len(subports)))
            for subport in subports:
                print("\tSubport ID: {} Enabled: {}".format(
                    subport.id,
                    subport.isForwardingEnabled))
