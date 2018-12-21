from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import unittest

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags
from fboss.system_tests.testutils.ip_conversion import ip_str_to_addr
from fboss.system_tests.testutils.subnet import get_subnet_from_ip_and_prefixlen
from fboss.system_tests.testutils.test_ip import get_random_test_ipv6
from neteng.fboss.ctrl.ttypes import IpPrefix
from neteng.fboss.ctrl.ttypes import UnicastRoute
from fboss.system_tests.test.ttypes import DeviceType
from neteng.fboss.ttypes import FbossBaseError
from libfb.py.decorators import retryable
from thrift.transport.TTransport import TTransportException
from ServiceRouter import TServiceRouterException


@test_tags("loopback", "run-on-diff", "trunk-stable")
@unittest.skip("Test is flaky - T38383753")
class LoopbackPing(FbossBaseSystemTest):
    """ Test whether host can ping other hosts' loopback interface
        """
    def test_loopback_ping(self):
        self.test_topology.min_hosts_or_skip(2)
        hosts = self.test_topology.hosts()
        # The chance that this ip is the exact same one with another ip in the
        # network is so low I decided not to have code checking for that
        loopback_address = get_random_test_ipv6()
        for host in hosts:
            self._test_each_host(host, hosts, loopback_address)

    def _test_each_host(self, host, hosts, loopback_address):
        self._setup_loopback_interface_in_host(host, loopback_address)
        unicast_route = self._add_route_to_switch(host, loopback_address)

        for other_host in hosts:
            if other_host == host:
                continue
            self._test_ping_over_loopback(host, other_host, loopback_address)

        self._cleanup(host, unicast_route)

    @retryable(num_tries=3, sleep_time=10, retryable_exs=[TTransportException,
        TServiceRouterException])
    def _test_ping_over_loopback(self, host, other_host, loopback_address):
        # Make sure each other host can ping that loopback address
        with other_host.thrift_client() as other_host_client:
            response = other_host_client.ping(loopback_address)
            self.assertTrue(response)

    @retryable(num_tries=3, sleep_time=10, retryable_exs=[TTransportException,
        TServiceRouterException])
    def _setup_loopback_interface_in_host(self, host, loopback_address):
        # Set up the loop1 loopback interface and add the loopback_address
        with host.thrift_client() as host_client:
            response = host_client.add_interface("loop1", DeviceType.LOOPBACK)
            self.assertTrue(response)
            response = host_client.add_address(loopback_address, "loop1")
            self.assertTrue(response)

    @retryable(num_tries=3, sleep_time=10, retryable_exs=[TTransportException,
        TServiceRouterException])
    def _delete_loopback_interface_in_host(self, host):
        # Delete loop1 loopback interface
        with host.thrift_client() as host_client:
            response = host_client.remove_interface("loop1")
            self.assertTrue(response)

    # Set up and add the route to the loopback address
    def _add_route_to_switch(self, host, loopback_address):
        subnet = get_subnet_from_ip_and_prefixlen(loopback_address, 64)
        dst = ip_str_to_addr(subnet)
        prefix = IpPrefix(
            ip=dst,
            prefixLength=64)
        nexthops = [ip_str_to_addr(next(host.ips()))]
        unicast_route = UnicastRoute(dest=prefix, nextHopAddrs=nexthops)
        return self._add_unicast_route(unicast_route)

    @retryable(num_tries=3, retryable_exs=[FbossBaseError,
        TServiceRouterException])
    def _add_unicast_route(self, unicast_route):
        with self.test_topology.switch_thrift() as client:
            client.addUnicastRoute(1, unicast_route)
        return unicast_route

    @retryable(num_tries=3, retryable_exs=[FbossBaseError,
        TServiceRouterException])
    def _delete_unicast_route(self, unicast_route):
        with self.test_topology.switch_thrift() as client:
            client.deleteUnicastRoute(1, unicast_route.dest)

    def _cleanup(self, host, unicast_route):
        # Clean up the newly added loopback interface
        self._delete_loopback_interface_in_host(host)

        # Remove the route
        self._add_unicast_route(unicast_route)
