/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"

#include <gtest/gtest.h>
#include <memory>

using namespace facebook::fboss;

class Jericho3AsicTest : public ::testing::Test {
 public:
  void SetUp() override {
    cfg::Range64 sysPortRange;
    sysPortRange.minimum() = 100;
    sysPortRange.maximum() = 144;
    asic_ = std::make_unique<Jericho3Asic>(
        cfg::SwitchType::VOQ,
        0,
        0,
        sysPortRange,
        folly::MacAddress("02:00:00:00:0F:0B"));
  }
  std::unique_ptr<Jericho3Asic> asic_;
};

TEST_F(Jericho3AsicTest, checkPortGroups) {
  const std::vector<std::pair<int, int>> kExpectedPortGroups{
      {1024, 1063}, {1064, 1103}, {1104, 1143}, {1144, 1183}};
  auto portGroups = asic_->getPortGroups();
  EXPECT_EQ(portGroups, kExpectedPortGroups);
}

TEST_F(Jericho3AsicTest, noCableLens) {
  EXPECT_FALSE(asic_->computePortGroupSkew({}).has_value());
}

TEST_F(Jericho3AsicTest, singlePortGroupCables) {
  std::map<PortID, uint32_t> singlePortGroup{
      {PortID(1024), 100},
      {PortID(1034), 200},
  };
  EXPECT_EQ(asic_->computePortGroupSkew(singlePortGroup), 0);
}
