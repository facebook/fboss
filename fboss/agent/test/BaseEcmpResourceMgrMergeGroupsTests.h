/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/test/BaseEcmpResourceManagerTest.h"

namespace facebook::fboss {

class BaseEcmpResourceMgrMergeGroupsTest : public BaseEcmpResourceManagerTest {
 private:
  void setupFlags() const override;

 public:
  int32_t getEcmpCompressionThresholdPct() const override {
    return 100;
  }
  std::optional<cfg::SwitchingMode> getBackupEcmpSwitchingMode()
      const override {
    return std::nullopt;
  }
  std::shared_ptr<EcmpResourceManager> makeResourceMgr() const override {
    return makeEcmpResourceManager(
        sw_->getState(), sw_->getHwAsicTable()->getL3Asics().front());
  }
  static constexpr auto kNumStartRoutes = 5;
  int numStartRoutes() const override {
    return kNumStartRoutes;
  }
  // Add next route - next prefix + next nhops. When we just want to
  // add a prefix, but don't care which one
  std::vector<StateDelta> addNextRoute();
  std::vector<RouteNextHopSet> defaultNhopSets() const;
  std::vector<RouteNextHopSet> nextNhopSets(
      int numSets = kNumStartRoutes) const;
  void SetUp() override;
};

} // namespace facebook::fboss
