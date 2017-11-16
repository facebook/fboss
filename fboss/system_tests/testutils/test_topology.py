from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from fboss.thrift_clients import FbossAgentClient, QsfpServiceClient
from fboss.netlink_manager.netlink_manager_client import NetlinkManagerClient
from fboss.system_tests.testutils.test_client import TestClient
from neteng.fboss.ttypes import FbossBaseError

from thrift.transport.TTransport import TTransportException

import logging
import unittest


class FbossTestException(BaseException):
    pass


class TestHost(object):
    """ FBOSS System Test Host Information
        For testing, a "host" is anything that can
        source and sink traffic that's connected to
        the switch we're testing

        The TestHost must be running the TestClient Thrift Service

        Example usage:

            config = FBOSSTestBaseConfig("switch142.awesomeco.com")

            h154 = TestHost("host154.awesomeco.com")
            h154.add_interface("eth0", "2381:db00:111:400a::154", 2)
            config.add_host(h154)
    """

    def __init__(self, dnsname, port=None):
        """ Init a TestHost
            @param dnsname: dnsname for host, e.g., 'fboss124.fb.com'
            @param port: the port of the TestClient thrift service
        """
        if port is None:
            port = TestClient.DEFAULT_PORT
        self.name = dnsname
        self.port = port    # the port of the TestClient thrift service
        # the interfaces we're to use in testing
        self.interface_map = {}

    def add_interface(self, intf_name, ip, switch_port=None):
        """ Add a testing interface on this host
            Tell the test that this host's interface is allowed for
            sourcing and syncing traffic.
            @param intf_name: a string (e.g., "eth0")
            @param ip: a v4 or v6 address as a string or None
            @param switch_port: an int or None - which switch port?

        """
        self.interface_map[intf_name] = {
            'ip': ip,
            'switch_port': switch_port  # 'None' means the config doesn't know
                                        # which switch port this host is
                                        # connected
        }

    def ips(self):
        for intf in self.interface_map.values():
            yield intf['ip']

    def intfs(self):
        return self.interface_map.keys()

    def thrift_client(self):
        return TestClient(self.name, self.port)

    def __str__(self):
        return self.name


class FBOSSTestTopology(object):
    """ FBOSS System Test Config
        Contains all of the state for which hosts, switches,
        and tests to run.

        For now, this is just for single switch testing.
    """

    def __init__(self, switch, port=None, qsfp_port=None):
        if port is None:
            port = FbossAgentClient.DEFAULT_PORT
        if qsfp_port is None:
            qsfp_port = QsfpServiceClient.DEFAULT_PORT
        self.port = port
        self.qsfp_port = qsfp_port
        self.log = logging.getLogger("__main__")
        self.switch = switch
        self.test_hosts = {}

    def add_host(self, host):
        """ This host is physically connected to this switch. """
        self.test_hosts[host.name] = host

    def remove_host(self, host):
        if host in self.test_hosts.values():
            del self.test_hosts[host.name]
        else:
            raise FbossTestException("host not in test topology: %s" % host)

    def min_hosts_or_skip(self, n_hosts):
        if len(self.test_hosts) < n_hosts:
            raise unittest.SkipTest("Test needs %d hosts, topology has %d",
                                    (n_hosts, len(self.test_hosts)))

    def verify_switch(self):
        """ Verify the switch is THRIFT reachable """
        try:
            with FbossAgentClient(self.switch, self.port) as client:
                client.keepalive()  # will throw FbossBaseError on failure
        except FbossBaseError:
            return False
        return True

    def verify_hosts(self, fail_on_error=False):
        """ Verify each host is thrift reachable
                if fail_on_error is false, just remove unreachable
                hosts from our setup with a warning.
        """
        bad_hosts = []
        for host in self.test_hosts.values():
            try:
                with TestClient(host.name, host.port) as client:
                    if not client.status():
                        bad_hosts.append(host)
                    else:
                        self.log.debug("Verified host %s" % host.name)
            except (FbossBaseError, TTransportException):
                bad_hosts.append(host)
        if bad_hosts:
            if fail_on_error:
                raise FbossBaseError("fail_on_error set and hosts down: %s" %
                                     " ".join(bad_hosts))
            else:
                for host in bad_hosts:
                    self.log.warning("Removing unreachable host: %s " %
                                        host.name)
                    self.remove_host(host)
                if len(self.test_hosts) == 0:
                    return False    # all hosts were bad
        return True

    def switch_thrift(self):
        return FbossAgentClient(self.switch, self.port)

    def qsfp_thrift(self):
        return QsfpServiceClient(self.switch, self.qsfp_port)

    def netlink_manager_thrift(self):
        return NetlinkManagerClient(self.switch, NetlinkManagerClient.DEFAULT_PORT)

    def hosts(self):
        return self.test_hosts.values()

    def hosts_thrift(self, hostname):
        self._valid_testhost(hostname)
        return self.test_hosts[hostname].thrift_client()

    def _valid_testhost(self, hostname):
        """ Is this host in our test topology? """
        if hostname not in self.test_hosts:
            raise FbossTestException("host not in test topology: %s" % hostname)

    def host_ips(self, hostname):
        self._valid_testhost(hostname)
        return self.test_hosts[hostname].ips()

    def get_switch_port_id_from_ip(self, host_binary_ip):
        # TODO(ashwinp): Add support for ARP.
        with self.switch_thrift() as sw_client:
                ndp_entries = sw_client.getNdpTable()
                for ndp_entry in ndp_entries:
                    if ndp_entry.ip == host_binary_ip:
                        return ndp_entry.port

        return None
