from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from fboss.system_tests.system_tests import FbossBaseSystemTest
from fboss.system_tests.testutils.ip_conversion import ip_addr_to_str


# By the time we get here, connectivity to the switch
# and the hosts has been verified
class L3InterfacesTest(FbossBaseSystemTest):
    """ Ping Each L3 Interface from Each host
    """
    def test_l3_interface_ping(self):
        with self.test_topology.switch_thrift() as sw_client:
            interfaces = sw_client.getAllInterfaces()
        for intf in interfaces.values():
            for host in self.test_topology.hosts():
                ip = ip_addr_to_str(intf.address[0].ip)
                for host_intf in host.intfs():
                    if ":" in ip:
                        # for v6, always ping with "%eth0" in address
                        ip = ip + "%%%s" % host_intf
                    self.log.info("Ping from %s to %s" % (host, ip))
                    with host.thrift_client() as client:
                        self.assertTrue(client.ping(ip))


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
