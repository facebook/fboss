from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags


@test_tags("qsfp")
class QsfpService(FbossBaseSystemTest):
    """ Thrift connect to the qsfp service and
        verify everything looks OK
    """
    def test_qsfp_present(self):
        qsfp_map = None
        # at least this many ports should be PRESENT
        min_sfps = len(self.test_topology.hosts())
        with self.test_topology.qsfp_thrift() as qsfp:
            qsfp_map = qsfp.getTransceiverInfo()
        self.assertTrue(len(qsfp_map) >= min_sfps)
        found_sfps = 0
        for _idx, sfp in qsfp_map.items():
            if sfp.present:
                found_sfps = found_sfps + 1
        self.log.info("Found %d present sfps" % found_sfps)
        self.assertTrue(found_sfps >= min_sfps)
