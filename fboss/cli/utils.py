# Copyright 2004-present Facebook. All Rights Reserved.
# @lint-avoid-python-3-compatibility-imports

import socket
import struct
import re
import sys

from math import log10
from facebook.network.Address.ttypes import BinaryAddress
from fbwhoami.fbwhoami import WhoAmI
from siteopseng.py.serf_util import SerfUtil
from facebook.core_systems.queries.ttypes import (BinaryExpression, Operator,
                                                  Query)

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

def ip_itop(addr):
    return ip_ntop(struct.pack(b"!L", addr))

def host_to_rack(host):
    """
    Retrieves the rack where the host lives
    """
    if host in ["::1", "127.0.0.1", "localhost"]:
        whoami = WhoAmI()
        datacenter = whoami["DEVICE_DATACENTER"]
        cluster = whoami["DEVICE_CLUSTER"]
        cluster_rack = whoami["DEVICE_CLUSTER_RACK"]
    else:
        # Check whether host is an IP or hostname
        for family in ((socket.AF_INET, "primary_ip"),
                       (socket.AF_INET6, "primary_ipv6")):
            try:
                socket.inet_pton(family[0], host)
                field = family[1]
                op = Operator.EQUAL
                break
            except socket.error:
                continue
        else:
            field = "name"
            op = Operator.LIKE
            host = "{0}%%".format(host)

        print("Polling Serf, this could take some time...")
        print()
        serf = SerfUtil().get_service('serf')
        query = Query([BinaryExpression(field, op, [host])])
        devices = serf.getDevicesByQuery(["*"], query)

        if len(devices) == 1:
            datacenter = devices[0].datacenter
            cluster = devices[0].cluster
            cluster_rack = devices[0].cluster_rack
        elif len(devices) > 1:
            raise Exception("Multiple devices found, please refine hostname")
        else:
            raise Exception("Could not find device in Serf")

    return '{0}:{1}:{2}'.format(datacenter, cluster, cluster_rack)

def host_for_bmc(host):
    """
    Derive the hostname for querying BMC
    """
    if host.startswith('rsw'):
        if re.search('oob', host):
            return host
        else:
            name = host.split('.', 1)
            return '{}-oob.{}'.format(name[0], name[1])
    return host

def mw_to_dbm(mw):
    if mw == 0:
        return 0.0
    else:
        return (10 * log10(mw))
