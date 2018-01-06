from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import unittest
import logging


class SystemTestsRunner(unittest.TextTestRunner):
    def __init__(self, list_tests=False, tests=None, log=logging, **kwargs):
        super(SystemTestsRunner, self).__init__(**kwargs)
        self.list_tests = list_tests
        self.log = log
        self.tests = tests

    def run(self, suite):
        if self.list_tests:
            self.log.info("List of available tests:")
            self._print_suite(suite)
            return
        if self.tests:
            new_suite = unittest.TestSuite()
            self._build_test_suite_from_names(new_suite, suite)
            return super(SystemTestsRunner, self).run(new_suite)
        return super(SystemTestsRunner, self).run(suite)

    def _print_suite(self, suite):
        for node in suite:
            if isinstance(node, unittest.suite.TestSuite):
                self.print_suite(node)
            else:
                print(node)

    def _build_test_suite_from_names(self, new_suite, suite):
        for node in suite:
            if isinstance(node, unittest.suite.TestSuite):
                self.build_test_suite_from_names(new_suite, node)
            # We need this logic both in here & in system_tests.py.
            # If users don't specify which test to run, this method won't be run
            # at all. If users do specify, the logic need to be here to catch
            # the FailedTest again.
            elif isinstance(node, unittest.loader._FailedTest):
                new_suite.addTest(node)
            # TestTopologyValidation always needed to be run
            else:
                if type(node).__name__ in self.tests or \
                   type(node).__name__ == 'TestTopologyValidation':
                    new_suite.addTest(node)
