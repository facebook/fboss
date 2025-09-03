// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/qsfp_service/PortManager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace facebook::fboss {

/*
 * MockPortManager is a simple test class that inherits from PortManager.
 * Since PortManager methods are not virtual, this class provides a way to
 * override behavior for testing by providing custom implementations.
 *
 * Note: This follows the same pattern as MockWedgeManager which only mocks
 * specific virtual methods. For PortManager, most methods are not virtual,
 * so we provide a simple inheritance-based approach for testing.
 */
class MockPortManager : public PortManager {
 public:
  MockPortManager(
      TransceiverManager* transceiverManager,
      std::unique_ptr<PhyManager> phyManager,
      const std::shared_ptr<const PlatformMapping> platformMapping,
      const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
          threads)
      : PortManager(
            transceiverManager,
            std::move(phyManager),
            platformMapping,
            threads) {}

  MOCK_METHOD1(getXphyInfo, phy::PhyInfo(PortID));
  MOCK_METHOD2(programExternalPhyPort, void(PortID, bool));
};

} // namespace facebook::fboss
