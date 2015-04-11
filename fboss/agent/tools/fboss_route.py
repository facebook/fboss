# Copyright (C) 2004-present Facebook. All Rights Reserved

from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
from __future__ import absolute_import

"""Add, change, or delete a route on FBOSS controller
"""

import contextlib
import ipaddr

from argparse import ArgumentParser, ArgumentError
from contextlib import contextmanager
from thrift.transport import TSocket
from thrift.protocol import TBinaryProtocol
from fboss.agent import FbossCtrl
from fboss.agent.ttypes import IpPrefix
from fboss.agent.ttypes import UnicastRoute
from facebook.network.Address.ttypes import BinaryAddress

DEFAULT_CLIENTID = 1

def parse_prefix(args):
    network = ipaddr.IPNetwork(args.prefix)
    return IpPrefix(ip=BinaryAddress(addr=network.ip.packed),
                    prefixLength=network.prefixlen)


def parse_nexthops(args):
    return [BinaryAddress(addr=ipaddr.IPAddress(nh).packed)
            for nh in args.nexthop]


def flush_routes(args):
    with get_client(args) as client:
        client.syncFib(args.client, [])


def add_route(args):
    prefix = parse_prefix(args)
    nexthops = parse_nexthops(args)
    with get_client(args) as client:
        client.addUnicastRoutes(
            args.client, [UnicastRoute(dest=prefix, nextHopAddrs=nexthops)])


def del_route(args):
    prefix = parse_prefix(args)
    with get_client(args) as client:
        client.deleteUnicastRoutes(args.client, [prefix])


@contextlib.contextmanager
def get_client(args, timeout=5.0):
    sock = TSocket.TSocket('localhost', args.port)
    sock.setTimeout(timeout * 1000)  # thrift timeout is in ms
    protocol = TBinaryProtocol.TBinaryProtocol(sock)
    transport = protocol.trans
    transport.open()
    client = FbossCtrl.Client(protocol)
    yield client
    transport.close()


if __name__ == '__main__':
    ap = ArgumentParser()
    ap.add_argument('--port', '-p', type=int, default=5909,
                    help='the controller thrift port')
    ap.add_argument('--client', '-c', type=int, default=DEFAULT_CLIENTID,
                    help='the client ID used to manipulate the routes')
    ap.add_argument('host',
                    help='the controller hostname')
    subparsers = ap.add_subparsers()

    flush_parser = subparsers.add_parser(
        'flush', help='flush all existing non-interface routes')
    flush_parser.set_defaults(func=flush_routes)

    add_parser = subparsers.add_parser(
        'add', help='add a new route or change an existing route')
    add_parser.set_defaults(func=add_route)
    add_parser.add_argument(
        'prefix', help='the route prefix, i.e. "1.1.1.0/24" or "2001::0/64"')
    add_parser.add_argument(
        'nexthop', nargs='+',
        help='the nexthops of the route, i.e "10.1.1.1" or "2002::1"')

    del_parser = subparsers.add_parser(
        'delete', help='delete an existing route')
    del_parser.set_defaults(func=del_route)
    del_parser.add_argument(
        'prefix', help='The route prefix, i.e. "1.1.1.0/24" or "2001::0/64"')

    args = ap.parse_args()
    args.func(args)
