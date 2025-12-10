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
            threads) {
    // By default, delegate to the real implementation
    // Tests can override with EXPECT_CALL if they need custom behavior
    ON_CALL(*this, programInternalPhyPorts(::testing::_))
        .WillByDefault([this](TransceiverID tcvrId) {
          PortManager::programInternalPhyPorts(tcvrId);
        });
    ON_CALL(*this, programExternalPhyPorts(::testing::_, ::testing::_))
        .WillByDefault([this](TransceiverID tcvrId, bool resetDataPath) {
          PortManager::programExternalPhyPorts(tcvrId, resetDataPath);
        });
  }

  MOCK_METHOD1(getXphyInfo, phy::PhyInfo(PortID));
  MOCK_METHOD1(programInternalPhyPorts, void(TransceiverID));
  MOCK_METHOD2(programExternalPhyPorts, void(TransceiverID, bool));

  // Wrapper functions for protected methods to enable direct testing
  std::unordered_set<TransceiverID> getTransceiversWithAllPortsInSet(
      const std::unordered_set<PortID>& ports) const {
    return PortManager::getTransceiversWithAllPortsInSet(ports);
  }

  // Helper methods for easier test access to cache data
  std::set<PortID> getInitializedPortsForTransceiver(
      TransceiverID tcvrId) const {
    const auto& cache = getTcvrToInitializedPortsForTest();
    auto it = cache.find(tcvrId);
    if (it != cache.end()) {
      return *(it->second->rlock());
    }
    return {};
  }

  bool hasTransceiverInCache(TransceiverID tcvrId) const {
    const auto& cache = getTcvrToInitializedPortsForTest();
    return cache.find(tcvrId) != cache.end();
  }
};

} // namespace facebook::fboss
