from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import unittest

from fboss.system_tests.system_tests import FbossBaseSystemTest


@unittest.skip("This test is correct, but code is currently broken -- fixme !!")
class PortStatusTest(FbossBaseSystemTest):
    """ Verify that for each port, that the internal state agrees with fb303 """
    def test_port_status_matchfb303(self):
        with self.test_topology.switch_thrift() as sw_client:
            for _pnum, pstate in sw_client.getAllPortInfo().items():
                self.assertEqual(pstate.operState,
                                 sw_client.getCounter("%s.up" % pstate.name))


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
