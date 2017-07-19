from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import socket

from facebook.network.Address.ttypes import BinaryAddress


def ip_addr_to_str(addr):
    '''
    :param addr: ip_types.BinaryAddress representing an ip address

    :returns: string representation of ip address
    :rtype: str or unicode
    '''

    family = socket.AF_INET if len(addr.addr) == 4 else socket.AF_INET6
    return socket.inet_ntop(family, addr.addr)


def ip_str_to_addr(addr_str):
    '''
    :param addr_str: ip address in string representation

    :returns: thrift struct BinaryAddress
    :rtype: ip_types.BinaryAddress
    '''

    # Try v4
    try:
        addr = socket.inet_pton(socket.AF_INET, addr_str)
        return BinaryAddress(addr=addr)
    except socket.error:
        pass

    # Try v6
    addr = socket.inet_pton(socket.AF_INET6, addr_str)
    return BinaryAddress(addr=addr)
