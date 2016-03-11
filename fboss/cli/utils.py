#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#
# @lint-avoid-python-3-compatibility-imports

import socket
import sys

from facebook.network.Address.ttypes import BinaryAddress

if sys.stdout.isatty():
    COLOR_RED = '\033[31m'
    COLOR_GREEN = '\033[32m'
    COLOR_RESET = '\033[m'
else:
    COLOR_RED = ''
    COLOR_GREEN = ''
    COLOR_RESET = ''

def ip_to_binary(ip):
    for family in (socket.AF_INET, socket.AF_INET6):
        try:
            data = socket.inet_pton(family, ip)
        except socket.error:
            continue
        return BinaryAddress(addr=data)
    raise socket.error('illegal IP address string: {}'.format(ip))


def ip_ntop(addr):
    if len(addr) == 4:
        return socket.inet_ntop(socket.AF_INET, addr)
    elif len(addr) == 16:
        return socket.inet_ntop(socket.AF_INET6, addr)
    else:
        raise ValueError('bad binary address %r' % (addr,))




