#!/usr/bin/env python3
#  Copyright (c) 2004-present, Facebook, Inc. All rights reserved.

from __future__ import division


import argparse
import random
import time

from fboss.cli.utils import utils
from fboss.thrift_clients import FbossAgentClient
from neteng.fboss.ctrl.ttypes import IpPrefix
from neteng.fboss.ctrl.ttypes import UnicastRoute

args = {
    'host': 'localhost',
    'port': 5909
}


class StressRouteInsertion(object):
    """ Measure latency of bulk route thrashing.

    Algorithm:
            1) Insert $entries randomly generated v6 route entries
            2) For loop = 1 to $loops:
                a) remove $percent percent of the routes, randomly
                b) generate a new set of $percent routes
                b) start the clock
                c) add new routes
                d) stop the clock and record the time

    The theory is this roughly emulates worst case of BGP sessions
    coming and going over time.  The $percent variable should
    correspond to a worst case guess as to the number of routes
    in a peering session.
    """
    def __init__(self, **kwargs):
        global args
        default_vals = {
            'host': None,
            'port': None,
            'entries': 32000,
            'percent': 10,
            'loops': 100,
            'minprefix': 16,
            'maxprefix': 64,
            'randseed': 0,     # we're going for pseudorandom, not real random
        }
        default_vals.update(kwargs)
        for kw, arg in default_vals.items():
            setattr(self, kw, arg)
        self.routes = {}
        self.generation = 1
        if (self.host is None) or (self.port is None):
            raise Exception("Test needs to specify 'host' and 'port' options")
        if (self.maxprefix > 120):
            raise Exception("Error: this tool is not yet smart" +
                            " enough to do maxprefix > 120")
        random.seed(self.randseed)
        self.client = FbossAgentClient(host=self.host, port=self.port)
        # a list of next hops; all routes point to same one: DROP
        self.nexthops = [utils.ip_to_binary(nh) for nh in []]
        self.client_id = 31336      # ID for our routes

    def generate_random_routes(self, n=None):
        # store routes as a dict to prevent duplications
        if n is None:
            n = self.entries
        routes = {}
        while len(routes) < n:
            routes[self.gen_rand_route()] = self.generation
        return routes

    def gen_rand_route(self):
        """ Generate a random IPv6 route as a string

        @NOTE: doesn't work for prefix > 120 bits"""
        prefix = random.randint(self.minprefix, self.maxprefix)   # inclusive
        r = ""
        for i in range(0, int(prefix / 4)):
            r += "{0:x}".format(random.randint(0, 15))
            if ((i + 1) % 4) == 0:
                r += ":"
        leftover = prefix - (i * 4)
        # this ensures we don't end with a ':' as well
        r += "{0:x}".format(random.randint(0, 15) & (pow(2, leftover) - 1))
        return r + "::1/{}".format(prefix)

    def insert_route(self, route):
        ip, prefix = route.split("/")
        addr = utils.ip_to_binary(ip)
        ipRoute = IpPrefix(ip=addr, prefixLength=prefix)
        uniRoute = UnicastRoute(dest=ipRoute, nextHopAddrs=self.nexthops)
        self.client.addUnicastRoute(self.client_id, uniRoute)

    def delete_route(self, route):
        ip, prefix = route.split("/")
        addr = utils.ip_to_binary(ip)
        ipRoute = IpPrefix(ip=addr, prefixLength=prefix)
        self.client.deleteUnicastRoute(self.client_id, ipRoute)

    def clean_up(self):
        for route in self.routes:
            self.delete_route(route)

    def run_test(self):
        """ Run actual test """
        print("Generating {} random routes with prefix between {} and {}".format(
            self.entries, self.minprefix, self.maxprefix))
        self.routes = self.generate_random_routes()

        print("Inserting initial routes into the switch...")
        start = time.clock()
        for route in self.routes:
            self.insert_route(route)
        stop = time.clock()
        print(" ... done : {} seconds - not the real test, but FYI".format(
            stop - start))

        target = (1 - (self.percent / 100)) * self.entries
        for loop in range(0, self.loops):
            print("--- Starting loop {}...".format(loop))
            print("Deleting {} routes".format(self.entries - target))
            while len(self.routes) > target:
                route = random.choice(list(self.routes.keys()))
                self.delete_route(route)
                del self.routes[route]
            print("Picking {} new routes".format(self.entries - target))
            new_routes = self.generate_random_routes(n=self.entries - target)
            print("Adding new routes")
            start = time.clock()
            for route in new_routes:
                self.routes[route] = loop
                self.insert_route(route)
            stop = time.clock()
            print("RESULT: {} seconds to add {} new routes".format(
                stop - start, self.entries - target))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Port Testing')
    parser.add_argument('--port', type=int,
                        help='Remote thrift TCP port ({})'.format(args['port']),
                        default=args['port'])
    parser.add_argument('--host',
                        help='Switch Mgmt intf ({})'.format(args['host']),
                        default=args['host'])
    args = parser.parse_args()
    try:
        test = StressRouteInsertion(host=args.host, port=args.port, entries=4,
                                    percent=50, loops=2)
        test.run_test()
    finally:
        test.clean_up()
