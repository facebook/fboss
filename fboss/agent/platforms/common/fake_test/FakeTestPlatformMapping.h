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

namespace facebook::fboss {

enum class FakeTestPlatformMappingType {
  STANDARD,
  DUAL_TRANSCEIVER,
  BACKPLANE,
  XPHY_BACKPLANE,
};

class FakeTestPlatformMapping : public PlatformMapping {
 public:
  explicit FakeTestPlatformMapping(
      std::vector<int> controllingPortIds,
      int portsPerSlot = 4,
      FakeTestPlatformMappingType mappingType =
          FakeTestPlatformMappingType::STANDARD);
  ~FakeTestPlatformMapping() = default;

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
      int portsPerSlot,
      bool twoTransceiversPerPort = false);

  void createPlatformMapping(int portsPerSlot);

  void createDualTransceiverPlatformMapping(int portsPerSlot);

  // Creates a platform mapping with direct NPU to Backplane connection
  // (no transceivers, no XPHY) - similar to Ladakh eth1/3/1 port style
  void createBackplanePlatformMapping();

  // Creates a platform mapping with NPU to XPHY to Backplane connection
  // (iphy + xphy + backplane) - similar to Ladakh eth1/43/1 port style
  void createXphyBackplanePlatformMapping();

  static phy::TxSettings getFakeTxSetting();
  static phy::RxSettings getFakeRxSetting();
};
} // namespace facebook::fboss
