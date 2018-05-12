from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import socket
import unittest

from fboss.system_tests.system_tests import FbossBaseSystemTest
from fboss.system_tests.testutils.ip_conversion import ip_str_to_addr
from fboss.system_tests.testutils.test_ip import get_random_test_ipv6
from fboss.system_tests.testutils.subnet import get_subnet_from_ip_and_prefixlen
from neteng.fboss.ctrl.ttypes import IpPrefix
from neteng.fboss.ctrl.ttypes import UnicastRoute
from neteng.fboss.ctrl.ttypes import StdClientIds


@unittest.skip('Broken test, UnicastRoute struct now has nextHops field as well')
class NetlinkManager(FbossBaseSystemTest):
    NETLINK_MANAGER_CLIENT_ID = StdClientIds.NETLINK_LISTENER

    def test_update_route(self):
        unicast_route = self._get_unicast_route()

        with self.test_topology.switch_thrift() as switch_client:
            fboss_routes = switch_client.getRouteTableByClient(
                self.NETLINK_MANAGER_CLIENT_ID)
            self.assertFalse(unicast_route in fboss_routes)

        with self.test_topology.netlink_manager_thrift() as netlink_client:
            response = netlink_client.addRoute(unicast_route, socket.AF_INET6)
            netlink_client._socket.close()
            self.assertEqual(response, 0)

        with self.test_topology.switch_thrift() as switch_client:
            fboss_routes = switch_client.getRouteTableByClient(
                self.NETLINK_MANAGER_CLIENT_ID)
            self.assertTrue(unicast_route in fboss_routes)

        with self.test_topology.netlink_manager_thrift() as netlink_client:
            response = netlink_client.deleteRoute(unicast_route, socket.AF_INET6)
            netlink_client._socket.close()
            self.assertEqual(response, 0)

        with self.test_topology.switch_thrift() as switch_client:
            fboss_routes = switch_client.getRouteTableByClient(
                self.NETLINK_MANAGER_CLIENT_ID)
            self.assertFalse(unicast_route in fboss_routes)

    def _get_unicast_route(self):
        prefix = self._get_prefix()
        nexthops = self._get_nexthops()

        return UnicastRoute(dest=prefix, nextHopAddrs=nexthops)

    def _get_prefix(self):
        str_test_ip = get_random_test_ipv6()
        prefixlen = 64
        str_subnet = get_subnet_from_ip_and_prefixlen(str_test_ip, prefixlen)
        binary_subnet = ip_str_to_addr(str_subnet)
        return IpPrefix(ip=binary_subnet,
                        prefixLength=prefixlen)

    def _get_nexthops(self):
        host = self.test_topology.hosts()[0]
        ip = next(host.ips())
        return [ip_str_to_addr(ip)]
