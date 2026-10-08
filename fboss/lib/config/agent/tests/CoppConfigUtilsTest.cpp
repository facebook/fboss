/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <vector>

#include <gtest/gtest.h>

#include "fboss/agent/hw/switch_asics/Qumran4DAsic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk5Asic.h"
#include "fboss/lib/config/agent/AclConfigUtils.h"
#include "fboss/lib/config/agent/CoppConfigUtils.h"

namespace facebook::fboss {
namespace {

cfg::SwitchInfo makeSwitchInfo(
    cfg::AsicType asicType,
    cfg::SwitchType switchType) {
  cfg::SwitchInfo switchInfo;
  switchInfo.switchType() = switchType;
  switchInfo.asicType() = asicType;
  switchInfo.switchIndex() = 0;
  switchInfo.switchMac() = "02:00:00:00:00:01";
  return switchInfo;
}

TEST(CoppConfigUtilsTest, AddsBasicNpuCoppConfigWithoutFlatAcls) {
  Tomahawk5Asic asic(
      0,
      makeSwitchInfo(cfg::AsicType::ASIC_TYPE_TOMAHAWK5, cfg::SwitchType::NPU));
  cfg::SwitchConfig config;
  ASSERT_TRUE(utility::setupDefaultAclTableGroups(config, asic));

  utility::addDefaultCpuQueueConfig(config, asic);
  utility::addDefaultCpuTrafficPolicyConfig(config, asic);

  ASSERT_EQ(config.cpuQueues()->size(), 4);
  EXPECT_EQ(
      std::vector<int16_t>({
          *config.cpuQueues()->at(0).id(),
          *config.cpuQueues()->at(1).id(),
          *config.cpuQueues()->at(2).id(),
          *config.cpuQueues()->at(3).id(),
      }),
      std::vector<int16_t>({9, 2, 1, 0}));
  EXPECT_EQ(
      *config.cpuQueues()->at(2).portQueueRate()->pktsPerSec()->maximum(), 200);
  EXPECT_EQ(
      *config.cpuQueues()->at(3).portQueueRate()->pktsPerSec()->maximum(), 100);

  ASSERT_TRUE(config.cpuTrafficPolicy().has_value());
  ASSERT_EQ(
      config.cpuTrafficPolicy()->rxReasonToQueueOrderedList()->size(), 13);
  ASSERT_EQ(
      config.cpuTrafficPolicy()->trafficPolicy()->matchToAction()->size(), 5);
  EXPECT_EQ(
      *config.cpuTrafficPolicy()
           ->rxReasonToQueueOrderedList()
           ->back()
           .rxReason(),
      cfg::PacketRxReason::UNMATCHED);

  EXPECT_TRUE(config.acls()->empty());
  ASSERT_EQ(config.aclTableGroups()->size(), 1);
  const auto& tables = config.aclTableGroups()->front().aclTables().value();
  ASSERT_EQ(tables.size(), 1);
  ASSERT_EQ(tables.front().aclEntries()->size(), 5);
  EXPECT_EQ(
      *tables.front().aclEntries()->front().name(),
      "cpuPolicing-CPU-Port-Mcast-v6");
}

TEST(CoppConfigUtilsTest, SkipsNonNpuSwitch) {
  Qumran4DAsic asic(
      0,
      makeSwitchInfo(cfg::AsicType::ASIC_TYPE_QUMRAN4D, cfg::SwitchType::VOQ));
  cfg::SwitchConfig config;

  utility::addDefaultCpuQueueConfig(config, asic);
  utility::addDefaultCpuTrafficPolicyConfig(config, asic);

  EXPECT_TRUE(config.cpuQueues()->empty());
  EXPECT_FALSE(config.cpuTrafficPolicy().has_value());
  EXPECT_TRUE(config.acls()->empty());
  EXPECT_FALSE(config.aclTableGroups().has_value());
}

} // namespace
} // namespace facebook::fboss
