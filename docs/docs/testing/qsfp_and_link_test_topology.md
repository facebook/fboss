# QSFP / Link Test Topology

Both QSFP and Link Tests run on a switch, with ports connected back on the same switch in a snake configuration. These are both single switch tests.

## General Topology Guidelines

To set up the test topology, follow these steps:

1. **Connect Transceivers**: Connect a pair of supported transceivers on the front panel.
2. **Connect Fiber**: Connect a supported fiber between them.
3. **Repeat for Port Coverage**: Repeat steps 1 and 2 to achieve the desired port coverage.
4. **Loopback Option**: Note that a loopback fiber back to the same transceiver is also supported.

![QSFP/Link Test Topology](/img/testing/qsfp_and_link_test_topology.png)

## Guidelines for Platform Vendor

To ensure comprehensive testing, follow this guideline:

1. **100% Port Coverage**: Achieve 100% port coverage using all the supported transceivers and transceiver vendors.

## Guidelines for Transceiver Vendor

To verify basic transceiver functionality and signal integrity, follow these guidelines:

1. **Minimum Transceivers**: Connect at least 2 transceivers being qualified for basic transceiver bring-up.
2. **Signal Integrity**: Create a topology with 100% port coverage to verify signal integrity.
