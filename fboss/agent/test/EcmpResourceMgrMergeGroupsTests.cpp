/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/BaseEcmpResourceManagerTest.h"

namespace facebook::fboss {

class EcmpResourceMgrMergeGroupsTest : public BaseEcmpResourceManagerTest {
 public:
  int32_t getEcmpCompressionThresholdPct() const override {
    return 100;
  }
  std::optional<cfg::SwitchingMode> getBackupEcmpSwitchingMode()
      const override {
    return std::nullopt;
  }
};

TEST_F(EcmpResourceMgrMergeGroupsTest, init) {}

TEST_F(EcmpResourceMgrMergeGroupsTest, reloadInvalidConfigs) {
  {
    // Both compression threshold and backup group type set
    auto newCfg = onePortPerIntfConfig(
        getEcmpCompressionThresholdPct(),
        cfg::SwitchingMode::PER_PACKET_RANDOM);
    EXPECT_THROW(sw_->applyConfig("Invalid config", newCfg), FbossError);
  }
  {
    // Changing compression threshold is not allowed
    auto newCfg = onePortPerIntfConfig(
        getEcmpCompressionThresholdPct() + 42, getBackupEcmpSwitchingMode());
    EXPECT_THROW(sw_->applyConfig("Invalid config", newCfg), FbossError);
  }
}
} // namespace facebook::fboss
