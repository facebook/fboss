#!/usr/bin/env python3

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags
from fboss.system_tests.testutils.packet import run_iperf
from fboss.system_tests.testutils.ip_conversion import ip_str_to_addr

import subprocess


@test_tags("cmdline", "new-test")
class FbossPortStatsCmdline(FbossBaseSystemTest):
    FBOSS_CMD = '/usr/local/bin/fboss'
    PORT_STATS_HEADER = 'Port Name   +Id In           bytes           uPkts \
    mcPkts     bcPkts       errs       disc Out           bytes           uPkts \
    mcPkts     bcPkts       errs       disc '
    INBYTES_COL = 3
    INPKTS_COL = 4
    OUTBYTES_COL = 10
    OUTPKTS_COL = 11

    def _get_port_stats(self, port):
        cmd = [FbossPortStatsCmdline.FBOSS_CMD,
                "-H", self.test_topology.switch.name,
                "port", "stats", str(port)]

        output = subprocess.check_output(cmd, encoding='utf-8')
        output = output.splitlines()
        self.assertTrue(output[0] == FbossPortStatsCmdline.PORT_STATS_HEADER)
        stats = output[1].split()

        return int(stats[FbossPortStatsCmdline.INBYTES_COL]),  \
                int(stats[FbossPortStatsCmdline.INPKTS_COL]),  \
                int(stats[FbossPortStatsCmdline.OUTBYTES_COL]),\
                int(stats[FbossPortStatsCmdline.OUTPKTS_COL])

    def test_fboss_port_stats_cmd(self):

        """
        Test the 'fboss port stats' command

        Steps:
            - Query in/out bytes/packets port stats for a server port.
            - Run iperf traffic from server (port under test) to another server.
            - Query in/out bytes/packets port stats for same sever port.
            - Stat counters should have been incremented.
        """

        self.test_topology.min_hosts_or_skip(2)

        host1 = self.test_topology.hosts()[0]
        host2 = self.test_topology.hosts()[1]

        host1_ip = next(host1.ips())
        host1_binary_ip = ip_str_to_addr(host1_ip)
        # Get Port corresponding to the server ip address
        port1 = self.test_topology.get_switch_port_id_from_ip(host1_binary_ip)

        inBytes1, inPkts1, outBytes1, outPkts1 = self._get_port_stats(port1)
        run_iperf(host1, host2, host1_ip)
        inBytes2, inPkts2, outBytes2, outPkts2 = self._get_port_stats(port1)

        self.assertTrue(inBytes2 > inBytes1 and inPkts2 > inPkts1 and
                outBytes2 > outBytes1 and outPkts2 > outPkts1)
