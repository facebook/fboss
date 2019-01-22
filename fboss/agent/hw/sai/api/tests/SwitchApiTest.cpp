/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class SwitchApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    switchApi = std::make_unique<SwitchApi>();
    switchId = fs->swm.create(FakeSwitch());
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<SwitchApi> switchApi;
  sai_object_id_t switchId;
};

TEST_F(SwitchApiTest, getNumPorts) {
  SwitchTypes::Attributes::PortNumber pn;
  EXPECT_EQ(switchApi->getAttribute(pn, switchId), 0);
  fs->pm.create(FakePort{{0}, 100000});
  fs->pm.create(FakePort{{1}, 25000});
  fs->pm.create(FakePort{{2}, 25000});
  fs->pm.create(FakePort{{3}, 25000});
  EXPECT_EQ(switchApi->getAttribute(pn, switchId), 4);
}

TEST_F(SwitchApiTest, testGetPortIds) {
  fs->pm.create(FakePort{{0}, 100000});
  fs->pm.create(FakePort{{1}, 25000});
  fs->pm.create(FakePort{{2}, 25000});
  fs->pm.create(FakePort{{3}, 25000});
  SwitchTypes::Attributes::PortNumber pn;
  auto numPorts =
      switchApi->getAttribute(SwitchTypes::Attributes::PortNumber(), switchId);
  std::vector<sai_object_id_t> v;
  v.resize(numPorts);
  SwitchTypes::Attributes::PortList pl(v);
  auto portIds = switchApi->getAttribute(pl, switchId);
  EXPECT_EQ(portIds.size(), numPorts);
}

TEST_F(SwitchApiTest, testSetMac) {
  folly::MacAddress newSrcMac("DE:AD:BE:EF:42:42");
  SwitchTypes::Attributes::SrcMac ma(newSrcMac);
  switchApi->setAttribute(ma, switchId);
  SwitchTypes::Attributes::SrcMac blank;
  EXPECT_EQ(switchApi->getAttribute(blank, switchId), newSrcMac);
}

TEST_F(SwitchApiTest, getDefaultVlanId) {
  EXPECT_EQ(
      switchApi->getAttribute(
          SwitchTypes::Attributes::DefaultVlanId(), switchId),
      1);
}

TEST_F(SwitchApiTest, setDefaultVlanId) {
  EXPECT_EQ(
      switchApi->setAttribute(
          SwitchTypes::Attributes::DefaultVlanId(42), switchId),
      SAI_STATUS_INVALID_PARAMETER);
}
