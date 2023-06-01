#
# Copyright 2004-present Facebook. All Rights Reserved.
#

namespace py3 neteng.fboss

include "fboss/agent/switch_config.thrift"

enum TestType {
  Hw = 1,
  Link = 2,
  Benchmark = 3,
  NWarmbootIteration = 4,
}

enum BootType {
  Coldboot = 1,
  Warmboot = 2,
}

struct TestConfig {
  1: TestType testType = TestType.Hw;
  2: BootType bootType = BootType.Warmboot;
  3: list<switch_config.AsicType> asic;
  4: string fromSdk;
  5: string toSdk;
  6: string regex;
}

struct SdkTestConfig {
  1: list<TestConfig> testConfig;
}
