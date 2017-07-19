from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from fboss.system_tests.testutils.test_topology import (TestHost,
                                                        FBOSSTestTopology)

""" Each test topology should return a fully populated FBOSSTestTopology object
"""


def generate_test_topology():
    """ Create and populate a FBOSSTestTopology() object """
    config = FBOSSTestTopology("rsw1aj.20.snc1")

    """ This example has two testing hosts """
    f154 = TestHost("fboss154.20.snc1")
    f154.add_interface("eth0", "2401:db00:111:400a::154")
    config.add_host(f154)

    f155 = TestHost("fboss155.20.snc1")
    f155.add_interface("eth0", "2401:db00:111:400a::155")
    config.add_host(f155)
    return config
