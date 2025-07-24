# FBOSS Platform BSP Tests

This test suite is intended to be a comprehensive test suite for the
FBOSS platform BSP. Tests are designed to cover each statement in the
two BSP specification documents:

* [BSP API Specification](/docs/platform/bsp_api_specification/)
* [BSP Development Requirements](/docs/platform/bsp_development_requirements/)

## Building

The test suite is a C++ binary built in the same way as the platform services.
Follow the instructions ["building fboss on docker containers"](/docs/build/building_fboss_on_docker_containers/)
and use the argument `--cmake-target bsp_tests`

## Running

Simply send the test binary to the switch under test and execute the binary.

* Note that the BSP you want to test must already be installed.
* To avoid conflicts with devices and files, ensure that no platform services
are running on the switch when testing.

## Test Failures

A test failure indicates that at least one of the requirements of the
specifications linked above is not met.

## Test Configuration

BSP Tests use the `platform_manager.json` file to know which devices to test,
but some additional information needs to be provided to the `bsp_tests.json`
configuration file. More details can be found in the [thrift file](https://github.com/facebook/fboss/blob/main/fboss/platform/bsp_tests/bsp_tests_config.thrift)
and [configs/sample/bsp_tests.json](https://github.com/facebook/fboss/tree/main/fboss/platform/configs/sample/bsp_tests.json)

### Expected Errors

In some cases it may not be possible for a specific hardware to meet all
specification requirements. In that case we provide an ExpectedErrors field
in the bsp tests config. Errors apply to one device and require a `reason`
string to be supplied to it. Currently supported error types can be found with
their documentation in the thrift file and sample config.

We do not plan to use this as a way to get around difficult requirements,
but if your hardware truly cannot meet a requirement for some reason please
reach out to your Meta contact.

## Contributing

Vendors are welcome to contribute to the test suite. The test suite lives
at [fboss/platform/bsp_tests](https://github.com/facebook/fboss/tree/main/fboss/platform/bsp_tests/cpp)
