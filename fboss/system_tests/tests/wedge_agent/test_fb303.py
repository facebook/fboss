from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags


@test_tags("run-on-diff", "trunk-stable")
class Fb303Tests(FbossBaseSystemTest):
    """ Verify we receive well-formatted IPv6 route advertisments
        On all hosts, on all interfaces
    """
    def test_aliveSince(self):
        with self.test_topology.switch_thrift() as sw_client:
            timestamp1 = sw_client.aliveSince()
            timestamp2 = sw_client.aliveSince()
        self.assertEqual(timestamp1, timestamp2)
