#!/usr/bin/env python3

from fboss.system_tests.system_tests import FbossBaseSystemTest
import fboss.system_tests.testutils.packet as packet
from fboss.system_tests.tests.copp_base import CoppBase


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
