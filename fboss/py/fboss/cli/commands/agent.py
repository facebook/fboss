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

from configerator.utils import config_contents_to_thrift
from fboss.cli.commands import commands as cmds
from fboss.cli.utils.utils import KEYWORD_CONFIG_RELOAD, KEYWORD_CONFIG_SHOW
from libfb.py.asyncio.thrift_utils import thrift_dumps
from neteng.fboss.switch_config.types import SwitchConfig
from neteng.fboss.ttypes import FbossBaseError


class AgentConfigCmd(cmds.FbossCmd):
    def _print_config(self, json_flag):
        with self._create_agent_client() as client:
            resp = client.getRunningConfig()

        if not resp:
            print("No Agent Config Info Found")
            return

        if json_flag:  # Output displayed as JSON if --json option is passed
            parsed = json.loads(resp)
            print(json.dumps(parsed, indent=4, sort_keys=True, separators=(",", ": ")))

        else:  # Display thrift struct output
            # Convert the received config into SwitchConfig thrift struct
            parsed = config_contents_to_thrift(resp, SwitchConfig)

            # Ensures that the output of thrift-py3 struct is readable
            print(thrift_dumps(parsed))

    def _reload_config(self):
        """Reload agent config without restarting"""
        with self._create_agent_client() as client:
            try:
                client.reloadConfig()
                print("Config reloaded")
                return 0
            except FbossBaseError as e:
                print("Fboss Error: " + e)
                return 2

    def run(self, cmd_type, json=False):
        if cmd_type == KEYWORD_CONFIG_SHOW:
            self._print_config(json)
        elif cmd_type == KEYWORD_CONFIG_RELOAD:
            self._reload_config()
        else:
            raise Exception(f"Unknown command `{cmd_type}`")
