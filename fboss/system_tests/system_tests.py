from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import argparse
import importlib
import logging
import os
import sys
import unittest


from fboss.system_tests.testutils.system_tests_runner import SystemTestsRunner
from fboss.system_tests.facebook.utils.ensemble_health_check_utils import (
    block_ensemble_and_create_task,
)
from neteng.netcastle.utils.basset_utils import BassetButler
from neteng.netcastle.test_case import test_tags

user_requested_tags = []

Defaults = {
    "config": 'test_topologies/example_topology.py',
    "console_log_level": 'info',   # very terse, for console log_level
    "file_log_level": 'debug',  # result/test-foo.log, more verbose
    "log_dir": "results",
    "log_file": "{dir}/result-{test}.log",
    "test_topology": None,
    "min_hosts": 2,
    "repeat-tests": 1,  # number of times we run tests before exiting
    "tags": user_requested_tags,
    "list_tests": False,
    "test_dirs": ["tests"],
    "sdk_list": ["chef"],
}


def generate_default_test_argparse(**kwargs):
    """ Put all command line args into a function, so that other
    programs (e.g., internal automation) can start with these args and build
    on them.
    """
    global Defaults
    log_levels = ['debug', 'info', 'warning', 'error', 'critical']
    parser = argparse.ArgumentParser(description='FBOSS System Tests', **kwargs)
    parser.add_argument('tests', nargs='*',
                        help="List of test classes to run. For example:\n"
                             "basset_test_runner.par [options] TestClass\n"
                             "basset_test_runner.par[options] TestClass1 TestClass2")
    parser.add_argument('--test_dirs', action='append',
                        help="List of directories to discover tests",
                        default=Defaults['test_dirs'])
    parser.add_argument('--override-test-dirs', action='store_const', const=[],
                        dest='test_dirs',
                        help="Ignore the default test directories, only "
                             "consider explicit --test_dir <dir> directories")
    parser.add_argument('--config', default=Defaults['config'])
    parser.add_argument('--log_dir', default=Defaults['log_dir'])
    parser.add_argument('--log_file', default=Defaults['log_file'])
    parser.add_argument('--min_hosts', type=int, default=Defaults['min_hosts'])
    parser.add_argument(
        '--console_log_level', type=str, default=Defaults['console_log_level'],
        choices=log_levels,
        help='Console log level ({})'.format(', '.join(log_levels)))
    parser.add_argument(
        '--file_log_level', type=str, default=Defaults['file_log_level'],
        choices=log_levels,
        help='File log level ({})'.format(', '.join(log_levels)))
    parser.add_argument('--repeat-tests', help="Times to repeatedly run tests",
                            type=int, default=Defaults['repeat-tests'])
    parser.add_argument('--tags',
                        help="Provide list of test tags, default is all tests "
                             "Example tags qsfp, port etc",
                        default=Defaults['tags'])
    parser.add_argument('--list_tests',
                        help="List all tests without running them",
                        action="store_true",
                        default=Defaults['list_tests'])
    parser.add_argument('--sdk_list', action='append',
                        help="SDKs to test against\n"
                             "('chef' for switch default SDK)",
                        default=Defaults['sdk_list'])

    return parser


def generate_default_test_options(**kwargs):
    """ Global system parameters for the test suite.
        This is conveniently formed from an argparse structure.
    """
    return generate_default_test_argparse(**kwargs).parse_args()


def dynamic_generate_test_topology(options):
    """ Read test topology from file, import it, and return
        the test_topology specified in generate_test_topology()

        This particular magic requires Python 3.5+
    """
    if hasattr(options, 'test_topology'):
        return options.test_topology
    spec = importlib.util.spec_from_file_location("config", options.config)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    options.test_topology = module.generate_test_topology()
    return options.test_topology


def setup_logging(options):
    """ Make sure that if a log_dir is set, it exists
    """
    if options.log_dir is not None:
        if not os.path.exists(options.log_dir):
            os.makedirs(options.log_dir)
    if not hasattr(options, "log"):
        # setup the console log if not done already
        # this is different from the per test file log
        options.log = logging.getLogger("__main__")
        options.log.setLevel(
            getattr(logging, options.console_log_level.upper()))


class FbossBaseSystemTest(unittest.TestCase):
    """ This Class is the base class of all Fboss System Tests """
    _format = "%(asctime)s.%(msecs)03d  %(name)-10s: %(levelname)-8s: %(message)s"
    _datefmt = "%m/%d/%Y-%H:%M:%S"

    def setUp(self):
        if not self.test_topology:
            raise Exception("optitest_topology not set - " +
                            "did you call run_tests()?")
        my_name = str(self.__class__.__name__ + "." + self._testMethodName)
        self.log = logging.getLogger(my_name)
        self.log.setLevel(logging.DEBUG)  # logging controlled by handlers
        logfile_opts = {'test': my_name, 'dir': self.log_dir}
        logfile = self.log_file.format(**logfile_opts)
        # close old log files
        for handler in self.log.handlers:
            self.log.removeHandler(handler)
            handler.close()
        # open one unique for each test class
        handler = logging.FileHandler(logfile, mode='w+')
        handler.setLevel(
            getattr(logging, self.file_log_level.upper()))
        handler.setFormatter(logging.Formatter(self._format, self._datefmt))
        self.log.addHandler(handler)
        self.test_hosts_in_topo = self.test_topology.number_of_hosts()
        # We have seen cases where previous testcase brings down the system
        # and all following testcaes fails. Instead we should skip a testcase
        # if system went to bad state.
        if not self._verify_topo_and_block_bad_ensemble():
            self.skipTest("Topology is in bad state, Skip Test")

    def tearDown(self):
        '''
        Make sure our topology is still in healthy state
        and no hosts got busted during test
        '''
        if not self._verify_topo_and_block_bad_ensemble():
            self.fail("Test Case broke topology healthy state")

    def _verify_topo_and_block_bad_ensemble(self):
        state, reason = self._get_topology_state()
        ensemble = self.test_topology.ensemble
        if not state:
            if not BassetButler().get_attr(ensemble, 'task-id'):
                ensemble = block_ensemble_and_create_task(
                    ensemble,
                    reason=reason,
                    logger=self.log)
                self.test_topology.ensemble = ensemble
            return False
        return True

    def _get_topology_state(self):
        self.log.info("Testing connection to switch")
        if not self.test_topology.verify_switch(log=self.log):
            self.log.info("Switch is not in good state")
            return False, "Switch is not in good state"
        if not self.test_topology.verify_hosts(
           min_hosts=self.test_topology.min_hosts, log=self.log):
            self.log.info("Hosts are not in good state")
            return False, "Hosts are not in good state"
        return True, ''


def frob_options_into_tests(suite, options):
    """ Make sure 'options' is available as a class variable
        to all of the tests.

        This is a horrible hack, but saves a lot of typing.
    """
    for test in suite._tests:
        if isinstance(test, unittest.suite.TestSuite):
            # recursively iterate through all of the TestSuites
            frob_options_into_tests(test, options)
        else:
            test.test_topology = options.test_topology
            test.log_file = options.log_file
            test.file_log_level = options.file_log_level
            test.log_dir = options.log_dir


def add_interested_tests_to_test_suite(tests, suite):
    if not isinstance(tests, unittest.suite.TestSuite):
        # so the test would not be run and there is no error.
        # The next 2 lines is to explicitly add these tests
        if isinstance(tests, unittest.loader._FailedTest) or not user_requested_tags:
            suite.addTest(tests)
            return
        # Add testcase which has user provided tag
        tags = test_tags.get_tags(tests)
        for tag in tags:
            if tag in user_requested_tags:
                suite.addTest(tests)
        return

    for test in tests:
        add_interested_tests_to_test_suite(test, suite)


def run_tests(options):
    """ Run all of the tests as described in options
    :options : a dict of testing options, as described above
    """
    setup_logging(options)
    # Skipping some work when we are just listing the tests
    if not options.list_tests:
        options.test_topology = dynamic_generate_test_topology(options)
    suite = unittest.TestSuite()
    for directory in options.test_dirs:
        if not os.path.exists(directory):
            raise Exception("Specified test directory '%s' does not exist" %
                            directory)
        options.log.info("Loading tests from test_dir=%s" % directory)
        testsdir = unittest.TestLoader().discover(start_dir=directory,
                                                  pattern='*test*.py')
        add_interested_tests_to_test_suite(testsdir, suite)
    frob_options_into_tests(suite, options)
    # useful for debugging flakey tests
    repeated_suite = unittest.TestSuite()
    for _i in range(options.repeat_tests):
        repeated_suite.addTests(suite)
    all_tests = []
    for test in suite:
        all_tests.append(test)
    options.log.info("""
    ===================================================
    ================ STARTING TESTS ===================
    ===================================================
    """)
    testRunner = SystemTestsRunner(list_tests=options.list_tests,
                                   tests=options.tests,
                                   log=options.log,
                                   verbosity=2)
    ret = testRunner.run(repeated_suite)
    options.log.info("""
    ===================================================
    ================  ENDING TESTS  ===================
    ===================================================
    """)
    return ret, all_tests


def main(args):
    options_parser = generate_default_test_argparse()
    options = options_parser.parse_args()
    run_tests(options)


if __name__ == '__main__':
    sys.exit(main(sys.argv))
