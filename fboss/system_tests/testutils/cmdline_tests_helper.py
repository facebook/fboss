from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import os
import subprocess


def check_binary_exist(cmd_path):
    # make sure binary exists
    return os.path.exists(cmd_path)


def check_subprocess_output(cmd):
    try:
        output = subprocess.check_output(cmd, encoding="utf-8")
    except Exception as e:
        raise Exception(
            "Something went wrong when executing \
            '{}' command. \n {}".format(cmd, e)
        )
    # make sure we got something
    if output is None:
        raise Exception(
            "'{}' command produced no output".format(cmd))
    return output
