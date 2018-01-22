from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags


@test_tags("ndp", "run-on-diff")
class V6NeightborAdvertisement(FbossBaseSystemTest):
    """ Verify we receive well-formatted IPv6 route advertisments
        On all hosts, on all interfaces
    """
    def test_v6_neighbor_advertisement(self):
        # TODO(rsher): run this outer loop in parallel to speed up test
        # Currently advertisments come every ~4 seconds, so this test takes
        # 4 * len(hosts) * ( n itnerfaces) seconds to run :-(
        for host in self.test_topology.hosts():
            with host.thrift_client() as client:
                for intf in host.intfs():
                    try:
                        client.startPktCapture(intf, "icmp6 and ip6[40] = 135")
                        self.log.info("Tesing host %s intf %s" % (host, intf))
                        pkts = client.getPktCapture(intf,
                                                    ms_timeout=5000,
                                                    maxPackets=1)
                        # Anything that matches the filter must be valid
                        self.assertTrue(pkts)
                    finally:
                        client.stopPktCapture(intf)
