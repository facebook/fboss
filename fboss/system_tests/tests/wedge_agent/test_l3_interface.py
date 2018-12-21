from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import unittest

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags
from fboss.system_tests.testutils.ip_conversion import ip_addr_to_str


def _append_outgoing_interface(ip, host_intf, is_ipv4=True):
    if ":" in ip and not is_ipv4:
        return (ip + "%%%s" % host_intf)
    elif "." in ip and is_ipv4:
        return ip
    else:
        return None


# By the time we get here, connectivity to the switch
# and the hosts has been verified
@test_tags("l3interface", "run-on-diff", "trunk-stable")
@unittest.skip("Test is flaky - T38383714")
class L3InterfacesTest(FbossBaseSystemTest):
    """
    Ping Each L3 Interface from Each host
    """
    def test_interface_ipv6_ping(self):
        '''
        Test switch l3 interface v6 ping only
        if connected host is v6 capable
        '''
        v6_capable_host_exist = False
        for host in self.test_topology.hosts():
            with host.thrift_client() as client:
                if client.get_v6_ip('eth0'):
                    v6_capable_host_exist = True
            if v6_capable_host_exist:
                self._test_interface_ip_ping(host, v4=False)
        if not v6_capable_host_exist:
            raise unittest.SkipTest("None of connected servers are v6 capable")

    def test_interface_ipv4_ping(self):
        '''
        Test switch l3 interface v4 ping only
        if connected host is v4 capable
        '''
        v4_capable_host_exist = False
        for host in self.test_topology.hosts():
            with host.thrift_client() as client:
                if client.get_v4_ip('eth0'):
                    v4_capable_host_exist = True
            if v4_capable_host_exist:
                self._test_interface_ip_ping(host, v4=True)
        if not v4_capable_host_exist:
            raise unittest.SkipTest("None of connected servers are v4 capable")

    def _test_interface_ip_ping(self, host, v4=True):
        with self.test_topology.switch_thrift() as sw_client:
            interfaces = sw_client.getAllInterfaces()
        for intf in interfaces.values():
            ip = ip_addr_to_str(intf.address[0].ip)
            for host_intf in host.intfs():
                ip = _append_outgoing_interface(ip, host_intf, v4)
                if ip is not None:
                    self.log.info("Ping from %s to %s" % (host, ip))
                    with host.thrift_client() as client:
                        self.assertTrue(client.ping(ip))


@test_tags("l3pairping", "run-on-diff", "trunk-stable")
@unittest.skip("Test is flaky - T38383680")
class L3AllPairsPing(FbossBaseSystemTest):
    """ Make sure each host can ping every other host """
    def test_all_pairs_ping(self):
        for src_host in self.test_topology.hosts():
            for dst_host in self.test_topology.hosts():
                if src_host == dst_host:
                    continue    # don't bother pinging ourselves
                for ip in dst_host.ips():
                    self.log.info("Ping from %s to %s" % (src_host, ip))
                    with src_host.thrift_client() as client:
                        self.assertTrue(client.ping(ip))
