from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import ipaddress


def get_subnet_from_ip_and_prefixlen(ip, prefixlen):
    '''
    :param ip(string): ip address
    :param prefixlen(int): prefix length

    :returns: string representation of the subnet

    '''
    prefix = "{}/{}".format(ip, str(prefixlen))
    network = ipaddress.ip_network(prefix, strict=False)
    return str(network.network_address)
