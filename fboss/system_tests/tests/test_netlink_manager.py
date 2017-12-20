from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import socket
import unittest

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags
from fboss.system_tests.testutils.ip_conversion import ip_str_to_addr
from neteng.fboss.ctrl.ttypes import IpPrefix
from neteng.fboss.ctrl.ttypes import UnicastRoute
from neteng.fboss.ctrl.ttypes import StdClientIds


@unittest.skip("Test is great, deployment is not. Should be fixed with D6577165")
@test_tags("netlink-manager")
class NetlinkManager(FbossBaseSystemTest):
    NETLINK_MANAGER_CLIENT_ID = StdClientIds.NETLINK_LISTENER

    def test_update_route(self):
        dst = ip_str_to_addr("2804:6082:2040:97f::")
        prefix = IpPrefix(
            ip=dst,
            prefixLength=64)

        host = next(iter(self.test_topology.hosts()))
        ip = next(host.ips())
        nexthops = [ip_str_to_addr(ip)]

        unicast_route = UnicastRoute(dest=prefix, nextHopAddrs=nexthops)

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
