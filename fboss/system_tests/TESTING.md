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
binary, you simply need to populate a FBOSSTestBaseConfig() object (see examples) and
run `./system_tests.py --config your_config.py`    TODO(rsher)

# New Development

All new tests should :
* inherit from 'FbossBaseSystemTest' found in system_tests.py
  ** If you think you are smarter than the defaults, minimally inherit from 'unittest.TestCase'
* live in system_tests/tests/
* match '*test*.py' to be autodiscovered by the test discovery algorithm


# Longer term goals

All of the test logic is independent of physical hardware and the goal
is to run the same tests inside a software SDK simulator.  That work is in progress.
