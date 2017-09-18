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
    config = FBOSSTestTopology("myswitch.example.com")  # DNS name of switch

    """ This example has two testing hosts """
    h1 = TestHost("testhost1.example.com")
    h1.add_interface("eth0", "2401:db00:111:400a::154")
    config.add_host(h1)

    h2 = TestHost("testhost2.example.com")
    h2.add_interface("eth0", "2401:db00:111:400a::155")
    config.add_host(h2)
    return config
