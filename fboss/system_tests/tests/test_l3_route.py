from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from fboss.system_tests.system_tests import FbossBaseSystemTest
from fboss.system_tests.testutils.ip_conversion import ip_str_to_addr
from fboss.system_tests.testutils.packet import make_packet
from neteng.fboss.ctrl.ttypes import IpPrefix
from neteng.fboss.ctrl.ttypes import UnicastRoute

import unittest


@unittest.skip("Test writing in progress")
class BasicL3RouteInstall(FbossBaseSystemTest):
    """ Install a route for a made up prefix with Host 2 as
        as next hop.  Then source a packet from Host 1 to the
        prefix and verify it arrives at host 2 """
    def test_basic_l3_route_install(self):
        # This route just needs to not ecplipse anything
        # in the test harness, e.g., the control channel
        ipRoute = IpPrefix(ip="1:2:3:4:5:6::", prefixLength=24)
        self.test_topology.min_hosts_or_skip(2)
        host1 = self.test_topology.hosts[0]
        host2 = self.test_topology.hosts[1]
        nextHops = [ip_str_to_addr(host2.ips[0])]
        route = UnicastRoute(dest=ipRoute, nextHopAddrs=nextHops)
        # Install the route
        with self.test_topology.switch_thrift as sw_client:
            sw_client.addUnicastRoute(self.client_id, route)
        target_intf = host2.intfs[0]
        beforePkt = make_packet(src_host=host1, dst_host=host2, ttl=32)
        afterPkt = make_packet(src_host=host1, dst_host=host2, ttl=31)
        # Now try the test packet
        with host2.thrift_client() as client2:
            with host1.thrift_client() as client1:
                client2.expect(target_intf, afterPkt)
                client1.sendPkt(beforePkt)
                client2.expectVerify(target_intf, afterPkt, timeout=1)
