#!/usr/bin/env python3

import threading
import time
import unittest

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags
from fboss.system_tests.testutils.ip_conversion import ip_str_to_addr
from fboss.system_tests.testutils.fb303_utils import get_fb303_counter
from neteng.fboss.ctrl.ttypes import PortOperState


@test_tags("port", "run-on-diff", "trunk-stable")
@unittest.skip("Test is flaky - T38384864")
class PortStatusTest(FbossBaseSystemTest):
    """ Verify that for each port, that the internal state agrees with fb303 """

    def test_port_status_matchfb303(self):
        self.test_topology.min_hosts_or_skip(1)
        mismatch_ports = []
        with self.test_topology.switch_thrift() as sw_client:
            all_port_info = sw_client.getAllPortInfo()
            for _pnum, pstate in all_port_info.items():
                is_port_up = pstate.operState == PortOperState.UP
                fb303_counter = get_fb303_counter(self, '%s.up' % (pstate.name))
                fb303_is_port_up = fb303_counter == 1
                if is_port_up != fb303_is_port_up:
                    mismatch_ports.append(
                        (pstate.name, pstate.operState, fb303_counter)
                    )
        self.assertEqual(
            len(mismatch_ports),
            0,
            msg="There are %d/%d mismatch ports: %s" % (
                len(mismatch_ports), len(all_port_info), ', '.join(
                    [
                        '%s (operState: %s, fb303_counter: %s' %
                        (name, is_port_up, fb303_is_port_up)
                        for name, is_port_up, fb303_is_port_up in mismatch_ports
                    ]
                )
            )
        )


@test_tags("port", "server_port_flap", "new-test")
@unittest.skip("Test broken - T24997025 to investigate further")
class ServerPortFlap(FbossBaseSystemTest):
    """ Verify that a server port flap is handled by fboss correctly and does
        not hang the system.
    """
    def test_server_port_flap(self):
        number_of_flaps = 1

        # Time for fboss to detect port down/up
        convergence_time_allowed = 10

        # We use the command ifup to bring the interface back up again and it
        # typically has a 20s delay.
        delay_time_buffer = 22  # in seconds

        # Run the test for all hosts in the topology.
        for host in self.test_topology.hosts():
            self.log.info("Testing host {}".format(host.name))
            # IP address on the server.
            host_binary_ip = ip_str_to_addr(next(host.ips()))

            # Interface on server.
            host_if = next(iter(host.intfs()))

            switch_port_id = None

            self.log.debug("Running port flap test on - {}".format(host))
            switch_port_id = self.test_topology.get_switch_port_id_from_ip(
                host_binary_ip)

            with self.test_topology.switch_thrift() as sw_client:
                port_info = sw_client.getPortInfo(switch_port_id)

                self.log.debug("Check that the port is up before test start.")
                self.assertEqual(port_info.operState, PortOperState.UP)

            thread = threading.Thread(target=self.threadFlapPort, args=(host_if,
                                      number_of_flaps, convergence_time_allowed,
                                      host))
            thread.start()

            time.sleep(5)

            with self.test_topology.switch_thrift() as sw_client:
                port_info = sw_client.getPortInfo(switch_port_id)
                self.assertEqual(port_info.operState, PortOperState.DOWN)

            time.sleep(delay_time_buffer + convergence_time_allowed)
            with self.test_topology.switch_thrift() as sw_client:
                port_info = sw_client.getPortInfo(switch_port_id)
                self.assertEqual(port_info.operState, PortOperState.UP)

    # TODO(linhbui): Add threading as a utility function.
    def threadFlapPort(self, host_if, number_of_flaps, delay_time_buffer, host):
        with host.thrift_client() as client:
            try:
                self.log.debug("Setting port down at {}".format(time.time()))
                client.flap_server_port(host_if, number_of_flaps,
                                        delay_time_buffer)

            # The call to the flap_server_port can cause an expected Socket read
            # failure since it brings the eth0 down.
            except Exception as e:
                self.log.info("Exception caught{}".format(e))
                pass


@test_tags("port", "new-test")
@unittest.skip("Test is flaky - T38384923")
class PortFECCheck(FbossBaseSystemTest):
    """ FEC should be enabled for all 100G ports """
    def test_fec_check_for_100g(self):
        Speed100Gbps = 100 * 1000
        found = False
        with self.test_topology.switch_thrift() as sw_client:
            for _pnum, pstate in sw_client.getAllPortInfo().items():
                if pstate.speedMbps == Speed100Gbps:
                    self.assertTrue(pstate.fecEnabled)
                    found = True
        if not found:
            raise unittest.SkipTest("Need 100G links in topology for this test")
