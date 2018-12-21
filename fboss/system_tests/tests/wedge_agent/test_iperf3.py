from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import unittest

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags
from fboss.system_tests.testutils.packet import run_iperf


@test_tags("iperf", "run-on-diff", "trunk-stable")
@unittest.skip("Tests is flaky - T38383504")
class Iperf3AllPairs(FbossBaseSystemTest):
    """ Make sure each host can ping every other host """
    def test_pair_iperf3(self):
        for src_host in self.test_topology.hosts():
            for dst_host in self.test_topology.hosts():
                if src_host == dst_host:
                    continue    # don't bother pinging ourselves
                for server_ip in src_host.ips():
                    self.log.info("iperf3 server up at %s" % server_ip)
                    results = run_iperf(src_host, dst_host, server_ip)[server_ip]
                    client_resp = results['client_resp']
                    server_resp = results['server_resp']
                    self.log.info('client result: %s' % client_resp)
                    self.log.info('server result: %s' % server_resp)
                    self.assertTrue(isinstance(server_resp, dict))
                    self.assertTrue(isinstance(client_resp, dict))
                    self.assertFalse('error' in server_resp.keys())
                    self.assertFalse('error' in client_resp.keys())
                    self.assertTrue(server_ip == client_resp['start']
                                    ['connecting_to']['host'])
