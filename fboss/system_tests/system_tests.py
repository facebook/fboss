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

Defaults = {
    "test_dirs": ['tests'],
    "config": 'test_topologies/example_topology.py',
    "log_level": logging.INFO,
    "log_dir": "results",
    "log_file": "{dir}/result-{test}.log",
    "test_topology": None,
    "min_hosts": 2
}


def generate_default_test_argparse(**kwargs):
    """ Put all command line args into a function, so that other
    programs (e.g., internal automation) can start with these args and build
    on them.
    """
    global Defaults
    parser = argparse.ArgumentParser(description='FBOSS System Tests', **kwargs)
    parser.add_argument('--test_dirs', default=Defaults['test_dirs'],
                                      nargs='*')
    parser.add_argument('--config', default=Defaults['config'])
    parser.add_argument('--log_dir', default=Defaults['log_dir'])
    parser.add_argument('--log_file', default=Defaults['log_file'])
    parser.add_argument('--min_hosts', default=Defaults['min_hosts'])
    parser.add_argument('--log_level', default=Defaults['log_level'])

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


class FbossBaseSystemTest(unittest.TestCase):
    """ This Class is the base class of all Fboss System Tests """
    _format = "%(asctime)s.%(msecs)03d  %(name)-10s: %(levelname)-8s: %(message)s"
    _datefmt = "%H:%M:%S"

    def setUp(self):
        if self.options is None:
            raise Exception("options not set - did you call run_tests()?")
        if (not hasattr(self.options, 'test_topology') or
                    self.options.test_topology is None):
            raise Exception("options.test_topology not set - " +
                            "did you call run_tests()?")
        self.test_topology = self.options.test_topology  # save typing
        my_name = str(self.__class__.__name__)
        self.log = logging.getLogger(my_name)
        self.log.setLevel(self.options.log_level)
        logfile_opts = {'test': my_name, 'dir': self.options.log_dir}
        logfile = self.options.log_file.format(**logfile_opts)
        # close old log files
        for handler in self.log.handlers:
            self.log.removeHandler(handler)
            handler.close()
        # open one unique to this class of tests
        handler = logging.FileHandler(logfile, mode='w+')
        handler.setFormatter(logging.Formatter(self._format, self._datefmt))
        self.log.addHandler(handler)


class TestTopologyValidation(FbossBaseSystemTest):
    def test_topology_sanity(self):
        self.log.info("Testing connection to switch")
        self.assertTrue(self.test_topology.verify_switch())
        self.log.info("Testing connection to hosts")
        self.assertTrue(self.test_topology.verify_hosts())


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


def run_tests(options):
    """ Run all of the tests as described in options
    :options : a dict of testing options, as described above
    """
    setup_logging(options)
    options.test_topology = dynamic_generate_test_topology(options)
    suite = unittest.TestSuite()
    # this test needs to run first
    suite.addTest(TestTopologyValidation('test_topology_sanity'))
    for directory in options.test_dirs:
        testsdir = unittest.TestLoader().discover(start_dir=directory,
                                                  pattern='*test*.py')
        suite.addTests(testsdir)
    frob_options_into_tests(suite, options)
    return unittest.TextTestRunner(verbosity=2).run(suite)


def main(args):
    options_parser = generate_default_test_argparse()
    options = options_parser.parse_args()
    run_tests(options)


if __name__ == '__main__':
    sys.exit(main(sys.argv))
