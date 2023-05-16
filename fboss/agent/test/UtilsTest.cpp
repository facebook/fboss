/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/Utils.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;
class UtilsTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Setup a default state object
    auto config = testConfigA();
    handle = createTestHandle(&config);
    sw = handle->getSw();
  }
  SwSwitch* sw{nullptr};
  std::unique_ptr<HwTestHandle> handle{nullptr};
};

TEST_F(UtilsTest, getIP) {
  EXPECT_NO_THROW(getAnyIntfIP(sw->getState()));
  EXPECT_NO_THROW(getSwitchIntfIP(sw->getState(), InterfaceID(1)));
}

TEST_F(UtilsTest, getIPv6) {
  EXPECT_NO_THROW(getAnyIntfIPv6(sw->getState()));
  EXPECT_NO_THROW(getSwitchIntfIPv6(sw->getState(), InterfaceID(1)));
}

TEST_F(UtilsTest, getPortsForInterface) {
  EXPECT_GT(getPortsForInterface(InterfaceID(1), sw->getState()).size(), 0);
}
