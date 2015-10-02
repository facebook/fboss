#!/usr/bin/env python
# Copyright (C) 2004-present Facebook. All Rights Reserved

from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
from __future__ import absolute_import

"""Add, change, or delete a route on FBOSS controller
"""

import contextlib
import ipaddr
import pdb

from argparse import ArgumentParser, ArgumentError
from contextlib import contextmanager
from thrift.transport import TSocket
from thrift.protocol import TBinaryProtocol
from fboss.ctrl import FbossCtrl
from fboss.ctrl.ttypes import IpPrefix
from fboss.ctrl.ttypes import UnicastRoute
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

def list_intf(args):
    details = args.details
    with get_client(args) as client:
        #for intf in client.getInterfaceList(): 
        for idx, intf in client.getAllInterfaces().iteritems(): 
            print ("L3 Interface %d: %s" %  (idx, str(intf)))

def list_routes(args):
    details = args.details
    with get_client(args) as client:
        for route in client.getRouteTable(): 
            print ("Route %s" %  route)

def list_optics(args):
    details = args.details
    with get_client(args) as client:
        for key,val in client.getTransceiverInfo([3]): 
            print ("Optic %d: %s" %  (key, str(val)))

@contextlib.contextmanager
def get_client(args, timeout=5.0):
    sock = TSocket.TSocket(args.host, args.port)
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
    ap.add_argument('--host',
                    help='the controller hostname', default='localhost')
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

    list_parser = subparsers.add_parser(
        'list_intf', help='list switch interfaces')
    list_parser.set_defaults(func=list_intf)
    list_parser.add_argument(
        '--details', action='store_true', help='List all information about the interface', default=False)

    list_route_parser = subparsers.add_parser(
        'list_route', help='list switch routes')
    list_route_parser.set_defaults(func=list_routes)
    list_route_parser.add_argument(
        '--details', action='store_true', help='List all information about the routes', default=False)

    list_optic_parser = subparsers.add_parser(
        'list_optic', help='list switch optics')
    list_optic_parser.set_defaults(func=list_optics)
    list_optic_parser.add_argument(
        '--details', action='store_true', help='List all information about the optics', default=False)


    args = ap.parse_args()
    args.func(args)
