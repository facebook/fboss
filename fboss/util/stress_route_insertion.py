#!/usr/bin/env python3
# Copyright (c) 2004-present, Facebook, Inc. All rights reserved.

import argparse
import random
import time

from fboss.cli.utils import utils
from fboss.py.fboss.thrift_clients import (
    PlainTextFbossAgentClientDontUseInFb as PlainTextFbossAgentClient,
)
from neteng.fboss.ctrl.ttypes import IpPrefix, UnicastRoute


class StressRouteInsertion:
    """Measure latency of bulk route thrashing.

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

    defaults = {
        "host": "localhost",
        "port": 5909,
        "entries": 4000,
        "percent": 10,
        "loops": 5,
        "minprefix": 16,
        "maxprefix": 64,
        "pause_on_exit": False,
        "randseed": 0,  # we're going for pseudorandom, not real random
    }

    def __init__(self, **kwargs):
        for kw, arg in StressRouteInsertion.defaults.items():
            setattr(self, kw, arg)
        for kw, arg in kwargs.items():
            setattr(self, kw, arg)
        self.routes = {}
        self.generation = 1
        if (self.host is None) or (self.port is None):
            raise Exception("Test needs to specify 'host' and 'port' options")
        if self.maxprefix > 120:
            raise Exception(
                "Error: this tool is not yet smart" + " enough to do maxprefix > 120"
            )
        random.seed(self.randseed)
        self.client = PlainTextFbossAgentClient(host=self.host, port=self.port)
        # a list of next hops; all routes point to same one: DROP
        self.nexthops = [utils.ip_to_binary(nh) for nh in []]
        self.client_id = 31336  # ID for our routes

    def generate_random_routes(self, n=None):
        # store routes as a dict to prevent duplications
        if n is None:
            n = self.entries
        routes = {}
        while len(routes) < n:
            routes[self.gen_rand_route()] = self.generation
        return routes

    def gen_rand_route(self):
        """Generate a random IPv6 route as a string

        @NOTE: doesn't work for prefix > 120 bits"""
        prefix = random.randint(self.minprefix, self.maxprefix)  # inclusive
        r = ""
        for i in range(0, int(prefix / 4)):
            r += f"{random.randint(0, 15):x}"
            if ((i + 1) % 4) == 0:
                r += ":"
        leftover = prefix - (i * 4)
        # this ensures we don't end with a ':' as well
        r += f"{random.randint(0, 15) & (pow(2, leftover) - 1):x}"
        return r + f"::1/{prefix}"

    def insert_routes(self, routes):
        uniRoutes = []
        for route in routes:
            ip, prefix = route.split("/")
            addr = utils.ip_to_binary(ip)
            ipRoute = IpPrefix(ip=addr, prefixLength=prefix)
            uniRoutes.append(UnicastRoute(dest=ipRoute, nextHopAddrs=self.nexthops))
        self.client.addUnicastRoutes(self.client_id, uniRoutes)

    def delete_routes(self, routes):
        uniRoutes = []
        for route in routes:
            ip, prefix = route.split("/")
            addr = utils.ip_to_binary(ip)
            uniRoutes.append(IpPrefix(ip=addr, prefixLength=prefix))
        self.client.deleteUnicastRoutes(self.client_id, uniRoutes)

    def clean_up(self):
        print("Removing all routes")
        self.delete_routes(self.routes)
        self.client._socket.close()
        print("...done.")

    def run_test(self):
        """Run actual test"""
        print(
            "Generating {} random routes with prefix between {} and {}".format(
                self.entries, self.minprefix, self.maxprefix
            )
        )
        self.routes = self.generate_random_routes()
        print("... done.")

        print("Inserting initial routes into the switch...")
        start = time.clock()
        self.insert_routes(self.routes)
        stop = time.clock()
        print(f" ... done : {stop - start} seconds - not the real test, but FYI")

        target = (1 - (self.percent / 100)) * self.entries
        for loop in range(0, self.loops):
            print(f"--- Starting loop {loop}...")
            print(f"Deleting {self.entries - target} routes")
            delete_routes = []
            while len(self.routes) > target:
                route = random.choice(list(self.routes.keys()))
                delete_routes.append(route)
                del self.routes[route]
            self.delete_routes(delete_routes)
            print(f"Picking {self.entries - target} new routes")
            new_routes = self.generate_random_routes(n=self.entries - target)
            print("Adding new routes")
            start = time.clock()
            for route in new_routes:
                self.routes[route] = loop
            self.insert_routes(new_routes)
            stop = time.clock()
            print(
                "RESULT: {} seconds to add {} new routes".format(
                    stop - start, self.entries - target
                )
            )
        if self.pause_on_exit:
            input("\n\n\nTest Done -- press return to cleanup: ")


def main() -> None:
    parser = argparse.ArgumentParser(description="Port Testing")
    parser.add_argument(
        "--port",
        type=int,
        help="Remote thrift TCP port ({})".format(
            StressRouteInsertion.defaults["port"]
        ),
        default=StressRouteInsertion.defaults["port"],
    )
    parser.add_argument(
        "--host",
        help="Switch Mgmt intf ({})".format(StressRouteInsertion.defaults["host"]),
        default=StressRouteInsertion.defaults["host"],
    )
    parser.add_argument(
        "--entries",
        type=int,
        help="Number of routes to insert ({})".format(
            StressRouteInsertion.defaults["entries"]
        ),
        default=StressRouteInsertion.defaults["entries"],
    )
    parser.add_argument(
        "--percent",
        type=int,
        help="Percent of routes to churn each loop ({})".format(
            StressRouteInsertion.defaults["percent"]
        ),
        default=StressRouteInsertion.defaults["percent"],
    )
    parser.add_argument(
        "--loops",
        type=int,
        help="Number of times to loop through the algorithm ({})".format(
            StressRouteInsertion.defaults["loops"]
        ),
        default=StressRouteInsertion.defaults["loops"],
    )
    parser.add_argument(
        "--pause_on_exit",
        action="store_true",
        help="Wait for key press before cleanup ({})".format(
            StressRouteInsertion.defaults["pause_on_exit"]
        ),
        default=StressRouteInsertion.defaults["pause_on_exit"],
    )
    args = parser.parse_args()
    try:
        test = StressRouteInsertion(**args.__dict__)
        test.run_test()
    finally:
        test.clean_up()


if __name__ == "__main__":
    main()
