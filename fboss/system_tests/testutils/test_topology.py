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
import time
import unittest


class FbossTestException(BaseException):
    pass


class RoleType(object):
    SINGLE_SWITCH = "single"
    CHASSIS = "chassis"


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
        return list(self.interface_map.keys())

    def thrift_client(self):
        return TestClient(self.name, self.port)

    def thrift_verify(self, retries, log=None):
        alive = False
        for i in range(retries):  # don't use @retry decorator - this is OSS
            try:
                with self.thrift_client() as client:
                    alive = client.status()
                    if alive:
                        break
                    if log:
                        log.debug("Failed to thrift verify %s: retry %d" %
                                    (self.name, i))
                    time.sleep(1)
            except Exception as e:
                msg = "Failed to thrift verify %s: " % self.name
                msg += " retry %d: Exception %s" % (i, str(e))
                if log:
                    log.debug(msg)
                else:
                    print(msg)
                time.sleep(1)
        if log:
            status = "ALIVE" if alive else "NOT alive"
            log.debug("Server %s is %s" % (self.name, status))
        if not alive and log:
            log.warning("Server %s failed to thrift verify" % self.name)
        return alive

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
        self.role_type = RoleType.SINGLE_SWITCH

    def add_host(self, host):
        """ This host is physically connected to this switch. """
        self.test_hosts[host.name] = host

    def remove_host(self, host_name):
        if host_name in self.test_hosts:
            del self.test_hosts[host_name]
        else:
            raise FbossTestException("host not in test topology: %s" % host_name)

    def number_of_hosts(self):
        return len(self.test_hosts)

    def min_hosts_or_skip(self, n_hosts):
        if len(self.test_hosts) < n_hosts:
            raise unittest.SkipTest("Test needs %d hosts, topology has %d" %
                                    (n_hosts, len(self.test_hosts)))
        if not self.verify_hosts(min_hosts=n_hosts):
            raise unittest.SkipTest("Not able to find %d good hosts" %
                                    (n_hosts))

    def verify_switch(self, log=None):
        """ Verify the switch is THRIFT reachable """
        if not log:
            log = self.log
        try:
            with FbossAgentClient(self.switch.name, self.port) as client:
                client.keepalive()  # will throw FbossBaseError on failure
        except (FbossBaseError, TTransportException):
            log.warning("Switch failed to thrift verify")
            return False
        return True

    def verify_hosts(self, fail_on_error=False, min_hosts=0, retries=10,
                     log=None):
        """ Verify each host is thrift reachable
                if fail_on_error is false, just remove unreachable
                hosts from our setup with a warning.
            zero min_hosts means we don't need any servers to run these tests
        """
        if not log:
            log = self.log
        bad_hosts = []
        for host in self.test_hosts.values():
            if not host.thrift_verify(retries=retries, log=log):
                bad_hosts.append(host.name)
        if bad_hosts:
            if fail_on_error:
                raise FbossBaseError("fail_on_error set and hosts down: %s" %
                                     " ".join(bad_hosts))
            else:
                for host_name in bad_hosts:
                    self.log.warning("Removing thrift-unreachable host: %s " %
                                        host_name)
                    self.remove_host(host_name)
        return len(self.test_hosts) >= min_hosts

    def switch_thrift(self):
        return FbossAgentClient(self.switch.name, self.port)

    def qsfp_thrift(self):
        return QsfpServiceClient(self.switch.name, self.qsfp_port)

    def netlink_manager_thrift(self):
        return NetlinkManagerClient(self.switch.name, NetlinkManagerClient.DEFAULT_PORT)

    def hosts(self):
        return list(self.test_hosts.values())

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


class FBOSSChassisTestTopology(object):
    """ FBOSS System Test Config
        Contains all of the state for which hosts, switches,
        and tests to run.

        This is specifically for devices that are composed of multiple
        subdevices, like in the case of backpack
    """

    def __init__(self, chassis, cards, port=None, qsfp_port=None):
        if port is None:
            port = FbossAgentClient.DEFAULT_PORT
        if qsfp_port is None:
            qsfp_port = QsfpServiceClient.DEFAULT_PORT
        self.port = port
        self.qsfp_port = qsfp_port
        self.log = logging.getLogger("__main__")
        self.chassis = chassis
        self.cards = self._generate_cards(cards)
        self.role_type = RoleType.CHASSIS

    def _generate_cards(self, cards):
        return [self._generate_card_topology(card) for card in cards]

    def _generate_card_topology(self, card):
        return FBOSSTestTopology(card, self.port, self.qsfp_port)

    def verify_switch(self):
        """ Verify the switch is THRIFT reachable """
        return all(card.verify_switch() for card in self.cards)

    def verify_hosts(self, min_hosts):
        return True

    def number_of_hosts(self):
        return 0
