#!/usr/bin/env python3

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags


def _verify_icmp6_type(self, icmp6_type, max_retries=10):
    # TODO(rsher): run this outer loop in parallel to speed up test
    # Currently advertisments come every ~5 seconds, so this test takes
    # 5 * len(hosts) * ( n itnerfaces) seconds to run :-(
    for host in self.test_topology.hosts():
        with host.thrift_client() as client:
            for intf in host.intfs():
                try:
                    client.startPktCapture(intf, "icmp6 and ip6[40] = %d" %
                                            icmp6_type)
                    # note that this is OSS code so no FB @retry decorator
                    for retry in range(max_retries):
                        self.log.info("Testing host %s intf %s: retry %d" %
                                            (host, intf, retry))
                        pkts = client.getPktCapture(intf,
                                                    ms_timeout=1000,
                                                    maxPackets=1)
                        if pkts:
                            break
                        # Anything that matches the filter must be valid
                    self.assertTrue(pkts, "Failed to get an icmp6_type %d msg for %s"
                                    % (icmp6_type, host.name))

                finally:
                    client.stopPktCapture(intf)


@test_tags("ndp", "new-test")   # see T31145754
class V6NeightborAdvertisement(FbossBaseSystemTest):
    """ Verify we receive well-formatted IPv6 neighbor advertisments
        On all hosts, on all interfaces
    """
    def test_v6_neighbor_advertisement(self):
        _verify_icmp6_type(self, 135)  # NDP


@test_tags("ndp", "run-on-diff", "trunk-stable")
class V6RouteAdvertisement(FbossBaseSystemTest):
    """ Verify we receive well-formatted IPv6 route advertisments
        On all hosts, on all interfaces
    """
    def test_v6_route_advertisement(self):
        _verify_icmp6_type(self, 134)  # RA
