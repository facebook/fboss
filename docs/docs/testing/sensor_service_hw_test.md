# sensor_service_hw_test


 sensor_service is one of the very important services running in FBOSS Switch Platforms. The service is expected to run stably for an extended period of time (more than one year.)

On the other hand, *sensor_service is very IO-heavy service.* The service constantly keeps polling all the sensors on-board, and communicates with other services in the system (FSDB/fan_service) as well as Meta’s Datacenter Infra services.

 There are two ways sensor_service shares data with other services.

1. Through FSDB as the central hub of all information
2. Direct Thrift call/response among services

 Note that HW Test of sensor_service tends to work as a unit test. Therefore most of the test cases are testing the internal features of the sensor_service, NOT the communication among services. **The only exception is the test for ensuring the correct operation of the fb303 counter upload logic.** This test is a Meta specific test, and will always fail if it’s executed in a OSS and/or stand-alone environment.

 Like other HW tests, Meta runs this test for qualifying every diffs (PRs), as well as in a node within the binary push pipeline.

 **Vendors are very welcome to implement test cases and send them to Meta as PR.** Please note that :
1. The test case should work on all platforms, not just one platform the vendor is developing.
2. Ensure using the existing test structure implemented in : `fboss/platform/fan_service/hw_test` directory.

The tests leverage the GTest framework.
