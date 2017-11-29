from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import unittest
from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags

JUMBO_FRAME_SIZE = 9000
PACKET_HEADER = 62


@test_tags("jumbo_frame")
class JumboFrameTest(FbossBaseSystemTest):
    def _is_topology_jumbo_frame_enabled(self):
        '''
        Verify if topology supports jumbo frames with out fragmentation.
        '''
        self._jumbo_frame_hosts = []

        # check if switch has jumbo frame MTU set
        with self.test_topology.switch_thrift() as sw_client:
            # (ToDo-prathyushapeddi): Have better way to obtain interface
            # MTU's. T22299283 will make it little better.
            resp = sw_client.getRunningConfig()
            if '\"mtu\": 9000' not in resp:
                return False

        # check if servers have jumbo MTU set,
        for host in self.test_topology.hosts():
            '''
            we will be doing jumbo frame testing between
            two servers, so consider getting two jumbo frame
            enabled servers
            '''
            with host.thrift_client() as client:
                mtu = client.get_interface_mtu('eth0')
            if mtu == JUMBO_FRAME_SIZE:
                self._jumbo_frame_hosts.append(host)
            if len(self._jumbo_frame_hosts) == 2:
                break
        if len(self._jumbo_frame_hosts) < 2:
            return False

        return True

    def _get_packet_frame_size(self):
        '''
        Send ping from 1 host and capture packets from other host
        '''
        with self._jumbo_frame_hosts[0].thrift_client() as client:
            with self._jumbo_frame_hosts[1].thrift_client() as cap_client:
                intf = list(self._jumbo_frame_hosts[1].intfs())[0]
                ip = list(self._jumbo_frame_hosts[1].ips())[0]

                # icmp[icmptype] = icmp-echo has not yet been implented. Taking
                # advantage of IPV6 layer with an index
                cap_client.startPktCapture(intf, "icmp6 and ip6[40] = 128")
                client.ping(ip, ['-c', '1', '-s',
                            str(JUMBO_FRAME_SIZE - PACKET_HEADER)])
                pkts = cap_client.getPktCapture(intf,
                                                ms_timeout=5000,
                                                maxPackets=1)
                return pkts[0].packet_length if pkts else 0

    def test_jumbo_frame_packet(self):
        '''
        Check if topology has jumbo frame enabled.
        Send ping with jumbo packet size
        Verify that there is no fragmentation
        '''
        if (self._is_topology_jumbo_frame_enabled()):
            self.log.debug("One host sends jumbo frame and other host captures packet")
            packet_length = self._get_packet_frame_size()
            self.assertEqual(packet_length, JUMBO_FRAME_SIZE)
        else:
            raise unittest.SkipTest("Need Jumbo Frame enabled topology for this test")
