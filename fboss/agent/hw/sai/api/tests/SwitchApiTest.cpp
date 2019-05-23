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

TEST_F(SwitchApiTest, setGetInit) {
  SwitchApiParameters::Attributes::InitSwitch init{true};
  switchApi->setAttribute(init, switchId);
  SwitchApiParameters::Attributes::InitSwitch blank{false};
  EXPECT_TRUE(switchApi->getAttribute(blank, switchId));
}

TEST_F(SwitchApiTest, getNumPorts) {
  SwitchApiParameters::Attributes::PortNumber pn;
  // expect the one global cpu port
  EXPECT_EQ(switchApi->getAttribute(pn, switchId), 1);
  fs->pm.create(FakePort{{0}, 100000});
  fs->pm.create(FakePort{{1}, 25000});
  fs->pm.create(FakePort{{2}, 25000});
  fs->pm.create(FakePort{{3}, 25000});
  // expect 4 created ports plus global cpu port
  EXPECT_EQ(switchApi->getAttribute(pn, switchId), 5);
}

TEST_F(SwitchApiTest, setNumPorts) {
  SwitchApiParameters::Attributes::PortNumber pn{100};
  EXPECT_THROW(
      saiCheckError(switchApi->setAttribute(pn, switchId)), SaiApiError);
}

TEST_F(SwitchApiTest, testGetPortIds) {
  fs->pm.create(FakePort{{0}, 100000});
  fs->pm.create(FakePort{{1}, 25000});
  fs->pm.create(FakePort{{2}, 25000});
  fs->pm.create(FakePort{{3}, 25000});
  SwitchApiParameters::Attributes::PortNumber pn;
  auto numPorts = switchApi->getAttribute(
      SwitchApiParameters::Attributes::PortNumber(), switchId);
  std::vector<sai_object_id_t> v;
  v.resize(numPorts);
  SwitchApiParameters::Attributes::PortList pl(v);
  auto portIds = switchApi->getAttribute(pl, switchId);
  EXPECT_EQ(portIds.size(), numPorts);
}

TEST_F(SwitchApiTest, testSetMac) {
  folly::MacAddress newSrcMac("DE:AD:BE:EF:42:42");
  SwitchApiParameters::Attributes::SrcMac ma(newSrcMac);
  switchApi->setAttribute(ma, switchId);
  SwitchApiParameters::Attributes::SrcMac blank;
  EXPECT_EQ(switchApi->getAttribute(blank, switchId), newSrcMac);
}

TEST_F(SwitchApiTest, getDefaultVlanId) {
  EXPECT_EQ(
      switchApi->getAttribute(
          SwitchApiParameters::Attributes::DefaultVlanId(), switchId),
      0);
}

TEST_F(SwitchApiTest, setDefaultVlanId) {
  EXPECT_EQ(
      switchApi->setAttribute(
          SwitchApiParameters::Attributes::DefaultVlanId(42), switchId),
      SAI_STATUS_INVALID_PARAMETER);
}

TEST_F(SwitchApiTest, getCpuPort) {
  auto cpuPort = switchApi->getAttribute(
      SwitchApiParameters::Attributes::CpuPort{}, switchId);
  EXPECT_EQ(cpuPort, 0);
}
