#!/usr/bin/env python3
import unittest

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags
import fboss.system_tests.testutils.packet as packet
from fboss.system_tests.tests.wedge_agent.copp_base import CoppBase


@test_tags("new-test")
@unittest.skip("Test is flaky - T38384443")
class CoppTest(CoppBase, FbossBaseSystemTest):
    """ Test that packets sent to CPU get the correct control plane policy
        applied.

        Parameterize the test verification function because we have
        other tests which can verify as well (and get code reuse).

        WARNING: this test can give false positives if you can't eliminate
        all ingress traffic in the test envirnoment to the DUT.
    """

    def send_pkt_verify_counter_bump(self, pkt, counter):
        return packet.send_pkt_verify_counter_bump(self, pkt, counter)

    def send_pkts_verify_counter_bump(self, pkts, counter):
        return packet.send_pkts_verify_counter_bump(self, pkts, counter)

    ###########
    # Automagically inherit all of the tests from
    # fboss.system_tests.tests.test_control_plane_policing.CoppBaseTest
    #
    # It's a little less obvious but a lot less typing then writing
    # each test definition twice
