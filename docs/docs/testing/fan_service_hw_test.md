# fan_service_hw_test


There are multiple HW test cases implemented to verify the functionality of the fan_service system service.

The HW tests are **executed against the actual fan_service running in multiple types of Switch HW platforms** that are supported by the fan_service.

The vendors and partners can leverage these tests as a way to do the sanity check on the following items:

1. Config files under development by vendors/partners
2. Core logic changes that will be shared with Meta as PR

Note that the HW tests are designed to quickly check the functionality of the fan_service. While the test cases can catch obvious issues and logical bugs, they are not designed to be executed as the tests to qualify HW during burn-in, RDT or 2C/4C tests.

*Meta runs these tests on every internal diffs (PRs) as well as in nodes within the push pipeline.*

**Vendors and partners are encouraged to contribute**, by adding HW Test test cases. Note that:
1. They are implemented in : `fboss/platform/fan_service/hw_test` directory
2. They are based on the GTest framework. GTest macros can be used
