// Copyright 2021-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/PortManager.h"
#include <folly/testing/TestUtil.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "fboss/lib/phy/facebook/bcm/minipack/MinipackPhyManager.h"
#include "fboss/qsfp_service/platforms/wedge/tests/MockWedgeManager.h"
#include "fboss/qsfp_service/test/MockManagerConstructorArgs.h"

namespace facebook::fboss {

class PortManagerTest : public ::testing::Test {
 public:
  void SetUp() override {
    initManagers();
  }

 protected:
  void initManagers(int numPortsPerModule = 4, bool setPhyManager = false) {
    const auto platformMapping =
        makeFakePlatformMapping(numModules, numPortsPerModule);

    // Using MinipackPhyManager as an example
    std::unique_ptr<MinipackPhyManager> phyManager =
        std::make_unique<MinipackPhyManager>(platformMapping.get());
    transceiverManager_ = std::make_shared<MockWedgeManager>(
        numModules, 4, platformMapping, nullptr /* threads */);
    portManager_ = std::make_unique<PortManager>(
        transceiverManager_.get(),
        setPhyManager ? std::move(phyManager) : nullptr,
        platformMapping,
        nullptr /* threads */);

    transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
        overrideMultiPortTcvrToPortAndProfile_);
  }
  void validatePortStatusNotInTestingOverride(PortID portId) {
    const auto& overrideStatusMap =
        portManager_->getOverrideAgentPortStatusForTesting();
    auto it = overrideStatusMap.find(portId);
    ASSERT_EQ(it, overrideStatusMap.end())
        << "Port ID " << portId << " found in override status map";
  }
  void validatePortStatusInTestingOverride(
      PortID portId,
      bool expectedEnabled,
      bool expectedUp) {
    const auto& overrideStatusMap =
        portManager_->getOverrideAgentPortStatusForTesting();
    auto it = overrideStatusMap.find(portId);
    ASSERT_NE(it, overrideStatusMap.end())
        << "Port ID " << portId << " not found in override status map";
    const auto& portStatus = it->second;

    ASSERT_EQ(portStatus.portEnabled, expectedEnabled)
        << "Port enabled status mismatch for port " << portId;
    ASSERT_EQ(portStatus.operState, expectedUp)
        << "Port operational state mismatch for port " << portId;
  }

  const TransceiverID tcvrId_ = TransceiverID(0);
  const PortID portId1_ = PortID(1);
  const PortID portId3_ = PortID(3);

  const cfg::PortProfileID multiPortProfile_ =
      cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL;
  const TransceiverManager::OverrideTcvrToPortAndProfile
      overrideMultiPortTcvrToPortAndProfile_ = {
          {tcvrId_,
           {
               {portId1_, multiPortProfile_},
               {portId3_, multiPortProfile_},
           }}};

  const int numModules = 1;
  std::shared_ptr<TransceiverManager> transceiverManager_;
  std::unique_ptr<PortManager> portManager_;
};

TEST_F(PortManagerTest, getLowestIndexedPortForTransceiverPortGroup) {
  // Single Tcvr - Single Port
  initManagers(1);
  ASSERT_EQ(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(1)),
      PortID(1));

  // Single Tcvr – Multi Port
  initManagers(4);
  ASSERT_EQ(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(1)),
      PortID(1));
  ASSERT_EQ(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(3)),
      PortID(1));

  // Multi Tcvr – Single Port (will be added once we validate other use cases)
}

TEST_F(PortManagerTest, getLowestIndexedTransceiverForPort) {
  // Single Tcvr – Single Port
  initManagers(1);
  ASSERT_EQ(
      portManager_->getLowestIndexedTransceiverForPort(PortID(1)),
      TransceiverID(0));

  // Single Tcvr – Multi Port
  initManagers(4);
  ASSERT_EQ(
      portManager_->getLowestIndexedTransceiverForPort(PortID(1)),
      TransceiverID(0));
  ASSERT_EQ(
      portManager_->getLowestIndexedTransceiverForPort(PortID(3)),
      TransceiverID(0));

  // Multi Tcvr – Single Port (will be added once we validate other use cases)
}

TEST_F(PortManagerTest, isLowestIndexedPortForTransceiverPortGroup) {
  // Single Tcvr - Single Port
  initManagers(1);
  ASSERT_TRUE(
      portManager_->isLowestIndexedPortForTransceiverPortGroup(PortID(1)));

  // Single Tcvr – Multi Port
  initManagers(4);
  ASSERT_TRUE(
      portManager_->isLowestIndexedPortForTransceiverPortGroup(PortID(1)));
  ASSERT_FALSE(
      portManager_->isLowestIndexedPortForTransceiverPortGroup(PortID(3)));

  // Multi Tcvr – Single Port (will be added once we validate other use cases)
}

TEST_F(PortManagerTest, getTransceiverIdsForPort) {
  // Single Tcvr – Single Port
  initManagers(1);
  ASSERT_THAT(
      portManager_->getTransceiverIdsForPort(PortID(1)),
      ::testing::ElementsAre(TransceiverID(0)));
  // Single Tcvr – Multi Port
  initManagers(4);
  ASSERT_THAT(
      portManager_->getTransceiverIdsForPort(PortID(1)),
      ::testing::ElementsAre(TransceiverID(0)));
  ASSERT_THAT(
      portManager_->getTransceiverIdsForPort(PortID(3)),
      ::testing::ElementsAre(TransceiverID(0)));
  // Multi Tcvr – Single Port (will be added once we validate other use cases)
}

// need to add more testing for tcvroverride too, any combinations that mihgt
// be relevant here
TEST_F(PortManagerTest, setBothPortsEnabledAndUpForTesting) {
  initManagers(4);
  portManager_->setOverrideAgentPortStatusForTesting(
      {PortID(1), PortID(3)}, {PortID(1), PortID(3)});

  validatePortStatusInTestingOverride(PortID(1), true, true);
  validatePortStatusInTestingOverride(PortID(3), true, true);
}

TEST_F(PortManagerTest, setBothPortsDisabledAndDownForTesting) {
  initManagers(4);
  portManager_->setOverrideAgentPortStatusForTesting({}, {});

  validatePortStatusInTestingOverride(PortID(1), false, false);
  validatePortStatusInTestingOverride(PortID(3), false, false);
}

TEST_F(PortManagerTest, setPortsMixedStatusForTesting) {
  initManagers(4);
  portManager_->setOverrideAgentPortStatusForTesting({PortID(1)}, {PortID(3)});

  validatePortStatusInTestingOverride(PortID(1), false, true);
  validatePortStatusInTestingOverride(PortID(3), true, false);
}

TEST_F(PortManagerTest, clearOverrideAgentPortStatusForTesting) {
  // needs to be modified to account for clear case
  initManagers(4);
  transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
      overrideMultiPortTcvrToPortAndProfile_);
  portManager_->setOverrideAgentPortStatusForTesting(
      {PortID(1), PortID(3)}, {PortID(1), PortID(3)});

  validatePortStatusInTestingOverride(PortID(1), true, true);
  validatePortStatusInTestingOverride(PortID(3), true, true);

  portManager_->setOverrideAgentPortStatusForTesting({}, {}, true);

  validatePortStatusNotInTestingOverride(PortID(1));
  validatePortStatusNotInTestingOverride(PortID(3));
}

TEST_F(PortManagerTest, getCachedXphyPorts) {
  initManagers(1, true);
  ASSERT_THAT(
      portManager_->getCachedXphyPortsForTest(),
      ::testing::UnorderedElementsAre(PortID(1)));
}

} // namespace facebook::fboss
