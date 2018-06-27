#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

import socket
import sys
import re

from typing import Tuple

from facebook.network.Address.ttypes import BinaryAddress

AGENT_KEYWORD = 'agent'
BGP_KEYWORD = 'bgp'
COOP_KEYWORD = 'coop'
SDK_KEYWORD = 'sdk'


def get_colors() -> Tuple[str, str, str]:
    if sys.stdout.isatty():
        return ('\033[31m', '\033[32m', '\033[m')
    return ('', '', '')


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


def port_sort_fn(port):
    if not port.name:
        return '', port.portId, 0, 0
    return port_name_sort_fn(port.name)


def port_name_sort_fn(port_name):
    m = re.match('([a-z][a-z][a-z])(\d+)/(\d+)/(\d)', port_name)
    if not m:
        return '', 0, 0, 0
    return m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4))


def make_error_string(msg):
    COLOR_RED, COLOR_GREEN, COLOR_RESET = get_colors()
    return COLOR_RED + msg + COLOR_RESET


def get_status_strs(status, is_present):
    ''' Get port status attributes '''

    COLOR_RED, COLOR_GREEN, COLOR_RESET = get_colors()
    attrs = {}
    admin_status = "Enabled"
    link_status = "Up"
    present = "Present"
    speed = ""
    if status.speedMbps:
        speed = "{}G".format(status.speedMbps // 1000)
    padding = 0

    color_start = COLOR_GREEN
    color_end = COLOR_RESET
    if not status.enabled:
        admin_status = "Disabled"
        speed = ""
    if not status.up:
        link_status = "Down"
        if status.enabled and is_present:
            color_start = COLOR_RED
        else:
            color_start = ''
            color_end = ''
    if is_present is None:
        present = "Unknown"
    elif not is_present:
        present = ""

    if color_start:
        padding = 10 - len(link_status)
    color_align = ' ' * padding

    link_status = color_start + link_status + color_end

    attrs['admin_status'] = admin_status
    attrs['link_status'] = link_status
    attrs['color_align'] = color_align
    attrs['present'] = present
    attrs['speed'] = speed

    return attrs


def get_qsfp_info_map(qsfp_client, qsfps, continue_on_error=False):
    if not qsfp_client:
        return {}
    try:
        return qsfp_client.getTransceiverInfo(qsfps)
    except Exception as e:
        if not continue_on_error:
            raise
        print(make_error_string(
            "Could not get qsfp info; continue anyway\n{}".format(e)))
        return {}
