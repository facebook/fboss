#!/bin/bash
# shellcheck source=/dev/null
cd /opt/fboss && source /opt/fboss/bin/setup_fboss_env
cd - > /dev/null || return
