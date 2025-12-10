# Link Test

## Overview

Link Tests are integration tests designed to verify the end-to-end functionality of network links. These tests involve both the `wedge_agent` and `qsfp_service`. The Link Test binary is essentially a `wedge_agent` binary with Google Test (gtest) integrated.


During platform bring-up, the agent hardware tests and QSFP hardware tests are the initial tests to pass, aiding in unit testing the ASIC and transceivers. Link Tests focus on the integration of these two software services and the end-to-end link bring-up. Since FSDB acts as the state-sharing service between the `wedge_agent` and `qsfp_service`, it is also run as part of Link Tests.

## Topology

Link Tests are conducted on a snake topology as defined in the [Link Test Topology document](/docs/testing/qsfp_and_link_test_topology/).

## Test Cases

Link Tests include the following scenarios:

- Testing link up on all cabled ports
- Flapping the links on the ASIC and asserting that they come back up every time
- Ensuring LLDP / Fabric reachability is established
- Testing layer 1 diagnostics reported by ASIC and Transceivers
- Verifying PRBS (Pseudo-Random Binary Sequence)
- Verifying warm-boot functionality
- And more

## Steps to Run Link Test

1. **Build Link Test Topology**: Set up the physical topology required for the tests.
2. **Generate Agent Config**: Create the agent configuration based on the cable connections.
3. **Generate QSFP Config**: Prepare the QSFP configuration.
4. **Build FBOSS qsfp_service and link_test Binaries**: Compile the `qsfp_service` and `link_test` binaries by following instructions on the [build page](/docs/build/building_fboss_on_docker_containers/).
5. **Use run_test.py to properly bringup qsfp_service and run link_test**: Follow instructions on the [Link tests page](/docs/build/packaging_and_running_fboss_hw_tests_on_switch/#link-tests)

By following these steps, you can ensure that the integration between the ASIC, transceivers, and the software services is functioning correctly and that the links are reliable.
