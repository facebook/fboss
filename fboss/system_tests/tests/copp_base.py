#!/usr/bin/env python3

import fboss.system_tests.testutils.packet as packet
from fboss.system_tests.testutils.ip_conversion import ip_addr_to_str

HIGH_PRI_QUEUE = 9
MID_PRI_QUEUE = 2
DOWNLINK_VLAN = 2000


"""
We want to re-use these tests with a few different verification functions,
but without completely duplicating all of the tests.  The test autodiscovery
gets confused if we inherit directly, so split out the functions into two
classes and use mixins/multiple inheritance to have our cake and eat it too.
"""


class CoppBase(object):
    """ Tests to verify our control plane policies (COPP)

        NOTE: this class must NOT inherit from unitest.TestCase or be in the
        same file as CoppTest or similar classes because it confuses the
        test discovery tool and causes tests to run multiple times.
    """
    def test_bgp_copp(self):
        """ Copp Map BGP traffic (dport=179) to high-priority
        """
        pkt = packet.gen_pkt_to_switch(self, dst_port=179)
        counter = "cpu.queue%d.in_pkts.sum" % HIGH_PRI_QUEUE
        self.send_pkt_verify_counter_bump(pkt, counter)

    def test_nonbgp_router_copp(self):
        """ Copp Map non-BGP traffic (dport!=179) to mid-pri
        """
        pkt = packet.gen_pkt_to_switch(self, dst_port=12345)
        counter = "cpu.queue%d.in_pkts.sum" % MID_PRI_QUEUE
        self.send_pkt_verify_counter_bump(pkt, counter)

    def test_all_router_ips(self):
        """ Does every router IP get COPP classified correctly?
        """
        with self.test_topology.switch_thrift() as switch_thrift:
            interfaces = switch_thrift.getAllInterfaces()
            self.assertIsNotNone(interfaces)
        test_bgp_pkts = []
        test_non_bgp_pkts = []
        for intf in interfaces.values():
            for prefix in intf.address:
                test_bgp_pkts.append(packet.gen_pkt_to_switch(self,
                                        dst_ip=ip_addr_to_str(prefix.ip),
                                        dst_port=179))
                test_non_bgp_pkts.append(packet.gen_pkt_to_switch(self,
                                         dst_ip=ip_addr_to_str(prefix.ip),
                                         dst_port=12345))
        # make sure each bgp-like pkt bumps the hi-pri counter
        self.send_pkts_verify_counter_bump(test_bgp_pkts,
                                           "cpu.queue%d.in_pkts.sum" %
                                                        HIGH_PRI_QUEUE)
        # make sure each non-bgp pkt bumps the mid-pri counter
        self.send_pkts_verify_counter_bump(test_non_bgp_pkts,
                                           "cpu.queue%d.in_pkts.sum" %
                                                        MID_PRI_QUEUE)
