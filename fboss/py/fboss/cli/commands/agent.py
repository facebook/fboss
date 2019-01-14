#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

import json

from fboss.cli.utils.utils import KEYWORD_CONFIG_SHOW, KEYWORD_CONFIG_RELOAD
from fboss.cli.commands import commands as cmds
from neteng.fboss.ttypes import FbossBaseError


class AgentConfigCmd(cmds.FbossCmd):
    def _print_config(self):
        with self._create_agent_client() as client:
            resp = client.getRunningConfig()

        if not resp:
            print("No Agent Config Info Found")
            return
        parsed = json.loads(resp)
        print(json.dumps(parsed, indent=4, sort_keys=True,
                         separators=(',', ': ')))

    def _reload_config(self):
        ''' Reload agent config without restarting '''
        with self._create_agent_client() as client:
            try:
                client.reloadConfig()
                print("Config reloaded")
                return 0
            except FbossBaseError as e:
                print('Fboss Error: ' + e)
                return 2

    def run(self, cmd_type):
        if (cmd_type == KEYWORD_CONFIG_SHOW):
            self._print_config()
        elif (cmd_type == KEYWORD_CONFIG_RELOAD):
            self._reload_config()
        else:
            raise Exception('Unknown command `{}`'.format(cmd_type))
