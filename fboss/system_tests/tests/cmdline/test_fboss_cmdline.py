from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import os
import subprocess

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags


@test_tags("cmdline", "run-on-diff")
class FbossCmdline(FbossBaseSystemTest):
    """ Does the 'fboss' tool work correctly? """
    FBOSS_CMD = '/usr/local/bin/fboss'
    HOSTNAME_COL = 2

    def test_fboss_hosts_cmd(self):
        """
        Test the 'fboss hosts' command

        Expected output (cut columns to fit 80 chars)

        $ fboss -H rsw1ak.21.snc1 hosts
Port          ID   Hostname                        Admin State  Link State ...
eth1/2/1      5    fboss174.21.snc1                   Enabled          Up
eth1/2/2      6    fboss175.21.snc1                   Enabled          Up
eth1/13/1     49   2401:db00:e011:511:1000::28        Enabled          Up
eth1/13/2     50   2401:db00:e011:511:1000::2a        Enabled          Up
eth1/14/1     53   eth1-21.csw21b.snc1                Enabled          Up
eth1/14/2     54   eth1-22.csw21b.snc1                Enabled          Up
eth1/15/1     57   eth1-21.csw21c.snc1                Enabled          Up
eth1/15/2     58   eth1-22.csw21c.snc1                Enabled          Up
eth1/16/1     61   eth1-21.csw21d.snc1                Enabled          Up
eth1/16/2     62   eth1-22.csw21d.snc1                Enabled          Up

        It's a bit much to test all of this, but a quick test to show
        it's not catestrophically dying is progress.

        It's probably not worth verifying all of the information here
        because the formatting will change, but we can always improve the
        test if need be.

        """
        # Does the binary exist?
        self.assertTrue(os.path.exists(FbossCmdline.FBOSS_CMD))
        cmd = [FbossCmdline.FBOSS_CMD,
                "-H", self.test_topology.switch.name,
                "hosts"
               ]
        output = subprocess.check_output(cmd, encoding="utf-8")
        self.assertTrue(output)  # make sure we got something
        output = output.splitlines()  # convern to array
        headings = output[0].split()
        self.assertEqual(headings[FbossCmdline.HOSTNAME_COL], "Hostname")
        target_hosts = {host.name: True for host in self.test_topology.hosts()}
        # as we step through each line, verify that all of the hosts
        # in the test topology appear UP and as expected
        for line in output[1:]:
            info = line.split()
            host = info[FbossCmdline.HOSTNAME_COL]
            if host in target_hosts:
                del target_hosts[host]
        # if this worked correctly, we should have found and removed
        # all of the target hosts from the dict
        self.assertEqual(len(target_hosts), 0)
