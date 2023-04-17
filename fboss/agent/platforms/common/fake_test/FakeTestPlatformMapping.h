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

#include "fboss/agent/platforms/common/PlatformMapping.h"

namespace facebook {
namespace fboss {

class FakeTestPlatformMapping : public PlatformMapping {
 public:
  explicit FakeTestPlatformMapping(
      std::vector<int> controllingPortIds,
      int portsPerSlot = 4);
  ~FakeTestPlatformMapping() {}

 private:
  std::vector<int> controllingPortIds_;

  // Forbidden copy constructor and assignment operator
  FakeTestPlatformMapping(FakeTestPlatformMapping const&) = delete;
  FakeTestPlatformMapping& operator=(FakeTestPlatformMapping const&) = delete;

  cfg::PlatformPortConfig getPlatformPortConfig(
      int portID,
      int startLane,
      int groupID,
      cfg::PortProfileID profileID);

  std::vector<cfg::PlatformPortEntry> getPlatformPortEntriesByGroup(
      int groupID,
      int portsPerSlot);
  static phy::TxSettings getFakeTxSetting();
  static phy::RxSettings getFakeRxSetting();
};
} // namespace fboss
} // namespace facebook
