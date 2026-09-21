#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

# pyre-unsafe

from fboss.cli.commands import commands as cmds


class ListHwObjectsCmd(cmds.FbossCmd):
    def run(self, hw_object_types, cached, phy_only, switch_asic_only):
        print_both = (not phy_only) and (not switch_asic_only)

        # PHY Objects:
        if phy_only or print_both:
            with self._create_qsfp_client() as client:
                print(client.listHwObjects(hw_object_types, cached))

        # Switch ASIC Objects:
        if switch_asic_only or print_both:
            with self._create_agent_client() as client:
                print(client.listHwObjects(hw_object_types, cached))
