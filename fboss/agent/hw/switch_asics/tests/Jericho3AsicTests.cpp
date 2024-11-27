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
    cfg::SwitchInfo switchInfo;
    switchInfo.systemPortRanges()->systemPortRanges()->push_back(sysPortRange);
    switchInfo.switchType() = cfg::SwitchType::VOQ;
    switchInfo.switchMac() = "02:00:00:00:0F:0B";
    switchInfo.switchIndex() = 0;
    asic_ = std::make_unique<Jericho3Asic>(0, switchInfo);
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

TEST_F(Jericho3AsicTest, fourPortGroupCables) {
  {
    // Skew b/w PG1 and PG2
    std::map<PortID, uint32_t> portGroups{
        // PG1
        {PortID(1024), 100},
        {PortID(1034), 200},
        // PG2
        {PortID(1065), 100},
        {PortID(1075), 50},
        // PG3
        {PortID(1104), 100},
        {PortID(1114), 150},
        // PG4
        {PortID(1144), 100},
        {PortID(1154), 100},

    };
    EXPECT_EQ(asic_->computePortGroupSkew(portGroups), 100);
  }
  {
    // Skew b/w PG1 and PG3
    std::map<PortID, uint32_t> portGroups{
        // PG1
        {PortID(1024), 100},
        {PortID(1034), 200},
        // PG2
        {PortID(1065), 100},
        {PortID(1075), 150},
        // PG3
        {PortID(1104), 100},
        {PortID(1114), 50},
        // PG4
        {PortID(1144), 100},
        {PortID(1154), 100},

    };
    EXPECT_EQ(asic_->computePortGroupSkew(portGroups), 100);
  }
  {
    // Skew b/w PG1 and PG4
    std::map<PortID, uint32_t> portGroups{
        // PG1
        {PortID(1024), 100},
        {PortID(1034), 200},
        // PG2
        {PortID(1065), 100},
        {PortID(1075), 150},
        // PG3
        {PortID(1104), 100},
        {PortID(1114), 150},
        // PG4
        {PortID(1144), 100},
        {PortID(1154), 50},

    };
    EXPECT_EQ(asic_->computePortGroupSkew(portGroups), 100);
  }
}
