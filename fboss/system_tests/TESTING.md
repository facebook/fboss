# FBOSS System Tests

This code runs system level testing for the FBOSS agent and associated tools.
Yes, it uses the python 'unittest' framework, but it tests the entire binary, not
just specific units.

# Testing Methodology

These tests assume that you have a hardware switch that you would like to test
and have some number of hosts physically, directly connected to it.  We then
run the TestServer daemon on each host to act as a programatic traffic generator
and run the switch through various permutations of:

* Configure switch
* Send some packets
* Verify the switch did the right thing

Some tests will require more hosts than others and the best tests are written
to work with a variable number of hosts.  Almost all meaningful tests require
at least two hosts connected to the switch.

# Running tests

Once the switch is running the FBOSS agent and the hosts are running the TestServer
binary, you simply need to populate a FBOSSTestBaseConfig() python object and
run `./system_tests.py --config your_config.py`.  For example, look at 
./test_topologies/example_topology.py for an example static test config.

This same testing framework is also optionally customizable for
dynamic/programatic/automatic test configuration.  For example, you
can write something like this:

  #!/usr/bin/python
  import system_tests
  import testutils.test_topology

  if __name__ == '__main__':
     argparse = system_tests.generate_default_test_argparse()
     # add custom command-line arguments here
     # ...
     options = argparse.parse_args()
     options.test_topology = your_custom_topology_generator()
     system_tests.run_test(options)

# Test Results

Test results are logged to two places: the console and a per-test logfile
(by default "results/result-TESTNAME.log").  There are commandline
flags to control the verbosity of what is logged to each location
(--console_log_level, --file_log_level ) as well as the default output
directory (--log_dir).  The idea is that the console should serve as
an uncluttered, terse summary of what passed and failed, where the logs
should be a detailed, permanent log suitable for debugging.

# New Development

All new tests should :
* inherit from 'FbossBaseSystemTest' found in system_tests.py
  ** If you think you are smarter than the defaults, minimally inherit from 'unittest.TestCase'
* live in system_tests/tests/
* match '*test*.py' to be autodiscovered by the test discovery algorithm

# Facebook Internal
* If you work at Facebook, look in the ./facebook directory for additonal automation
magic.


# Longer term goals

All of the test logic is independent of physical hardware and the goal
is to run the same tests inside a software SDK simulator.  That work is in progress.
