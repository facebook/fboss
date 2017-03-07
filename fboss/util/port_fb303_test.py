#!/usr/bin/env python3
#  Copyright (c) 2004-present, Facebook, Inc. All rights reserved.

import unittest

from fboss.thrift_clients import FbossAgentClient

# FIXME - make me a real parameterizable thingee
args = {
    'host': 'rsw1aa.21.snc1.facebook.com',
    'port': 5909
}


class PortStatusTest(unittest.TestCase):

    def setUp(self):
        self.client = FbossAgentClient(host=args['host'], port=args['port'])

    def test_port_status_matchfb303(self):
        """ Verify that for each port, it's internal status (down, up)
        matches what we're reporting to fb303.  See t14145409
        """
        for _pnum, pstate in self.client.getAllPortInfo().items():
            self.assertEqual(pstate.operState,
                             self.client.getCounter("%s.up" % pstate.name))

    def tearDown(self):
        # TODO(rsher) figure out better cleanup
        self.client.__exit__(None, None, None)


if __name__ == '__main__':
    unittest.main()
