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

user_requested_tags = []

Defaults = {
    "config": 'test_topologies/example_topology.py',
    "console_log_level": logging.INFO,   # very terse, for console log_level
    "file_log_level": logging.DEBUG,  # result/test-foo.log, more verbose
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


def _test_has_user_requested_tag(test_tags):
    for tag in test_tags:
        if tag in user_requested_tags:
            return True
    return False


def test_tags(*args):
    def fn(cls):
        if _test_has_user_requested_tag(list(args)):
            cls.valid_tags = True
        return cls
    return fn


def generate_default_test_argparse(**kwargs):
    """ Put all command line args into a function, so that other
    programs (e.g., internal automation) can start with these args and build
    on them.
    """
    global Defaults
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
    parser.add_argument('--console_log_level', default=Defaults['console_log_level'])
    parser.add_argument('--file_log_level', default=Defaults['file_log_level'])
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
        options.log.setLevel(options.console_log_level)


class FbossBaseSystemTest(unittest.TestCase):
    """ This Class is the base class of all Fboss System Tests """
    _format = "%(asctime)s.%(msecs)03d  %(name)-10s: %(levelname)-8s: %(message)s"
    _datefmt = "%m/%d/%Y-%H:%M:%S"

    TopologyIsSane = True

    def setUp(self):
        if self.options is None:
            raise Exception("options not set - did you call run_tests()?")
        if (not hasattr(self.options, 'test_topology') or
                    self.options.test_topology is None):
            raise Exception("options.test_topology not set - " +
                            "did you call run_tests()?")
        self.test_topology = self.options.test_topology  # save typing
        my_name = str(self.__class__.__name__ + "." + self._testMethodName)
        self.log = logging.getLogger(my_name)
        self.log.setLevel(logging.DEBUG)  # logging controlled by handlers
        logfile_opts = {'test': my_name, 'dir': self.options.log_dir}
        logfile = self.options.log_file.format(**logfile_opts)
        # close old log files
        for handler in self.log.handlers:
            self.log.removeHandler(handler)
            handler.close()
        # open one unique for each test class
        handler = logging.FileHandler(logfile, mode='w+')
        handler.setLevel(self.options.file_log_level)
        handler.setFormatter(logging.Formatter(self._format, self._datefmt))
        self.log.addHandler(handler)
        self.test_hosts_in_topo = self.test_topology.number_of_hosts()
        # We have seen cases where previous testcase brings down the system
        # and all following testcaes fails. Instead we should skip a testcase
        # if system went to bad state.
        if not self.TopologyIsSane:
            raise unittest.SkipTest("Topology is in bad state, Skip Test")

    def tearDown(self):
        '''
        Make sure our topology is still in healthy state
        and no hosts got busted during test
        '''
        if self.TopologyIsSane:     # don't test if things are already broken
            self.log.info("Testing connection to switch")
            self.TopologyIsSane = self.test_topology.verify_switch(log=self.log)
            self.assertTrue(self.TopologyIsSane)
            self.log.info("Testing connection to hosts")
            self.TopologyIsSane = \
                self.test_topology.verify_hosts(min_hosts=self.options.min_hosts,
                                                log=self.log)
            self.assertTrue(self.TopologyIsSane,
                            "Test broke connectivity to hosts?")


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
            test.options = options


def add_interested_tests_to_test_suite(tests, suite):
    if not isinstance(tests, unittest.suite.TestSuite):
        # If user uses tag and there is an import error on a test
        # the "valid_tags" attributes are never added,
        # so the test would not be run and there is no error.
        # The next 2 lines is to explicitly add these tests
        if isinstance(tests, unittest.loader._FailedTest):
            suite.addTest(tests)
            return
        # when user provides a tag , add testcases which has
        # valid tags and add all testcases when user do not
        # provide any tags
        if hasattr(tests, "valid_tags") or not user_requested_tags:
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
