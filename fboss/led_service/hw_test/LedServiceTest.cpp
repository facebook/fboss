/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "common/init/Init.h"
#include "fboss/agent/EnumUtils.h"
#include "fboss/led_service/LedManager.h"
#include "fboss/led_service/hw_test/LedServiceTest.h"

namespace facebook::fboss {

void LedServiceTest::SetUp() {
  // Create ensemble and initialize it
  ensemble_ = std::make_unique<LedEnsemble>();
  ensemble_->init();

  // Check if the LED manager is created correctly, the config file is loaded
  // and the LED is managed by service now
  auto ledManager = getLedEnsemble()->getLedManager();
  CHECK_NE(ledManager, nullptr);
  CHECK(ledManager->isLedControlledThroughService());
}

void LedServiceTest::TearDown() {
  // Remove ensemble and objects contained there
  ensemble_.reset();
}

std::vector<TransceiverID> LedServiceTest::getAllTransceivers(
    const PlatformMapping* platformMapping) const {
  std::vector<TransceiverID> transceivers;
  const auto& chips = platformMapping->getChips();
  for (auto chip : chips) {
    if (*chip.second.type() == phy::DataPlanePhyChipType::TRANSCEIVER) {
      auto tcvrID = TransceiverID(*chip.second.physicalID());
      transceivers.push_back(tcvrID);
    }
  }
  return transceivers;
}

/*
 * Set the LED state by updating the LED manager and check if the LED color
 * changes accordingly
 */
TEST_F(LedServiceTest, checkLedColorChange) {
  auto ledManager = getLedEnsemble()->getLedManager();

  // Test on the first module port
  // Use the ports max speed and profile
  auto platformMap = ledManager->getPlatformMapping();
  auto swPorts = platformMap->getSwPortListFromTransceiverId(0);
  CHECK_GT(swPorts.size(), 0);
  auto swPort = swPorts[0];

  // The setExternalLedState will throw because first update from FSDB has not
  // happened to the LedManager
  EXPECT_THROW(
      ledManager->setExternalLedState(
          swPort, PortLedExternalState::EXTERNAL_FORCE_OFF),
      FbossError);

  // Do the first update from FSDB to LedService
  auto maxSpeed = platformMap->getPortMaxSpeed(swPort);
  auto profile = platformMap->getProfileIDBySpeed(swPort, maxSpeed);
  LedManager::LedSwitchStateUpdate ledUpdate = {
      static_cast<short>(swPort),
      "",
      enumToName<cfg::PortProfileID>(profile),
      false};

  std::map<short, LedManager::LedSwitchStateUpdate> switchUpdate_0;
  switchUpdate_0[swPort] = ledUpdate;
  ledManager->updateLedStatus(switchUpdate_0);

  // Now the setting of external LED state should be successful
  EXPECT_NO_THROW(ledManager->setExternalLedState(
      swPort, PortLedExternalState::EXTERNAL_FORCE_OFF));

  // Verify link Down, the expected LED color is OFF
  auto offLedColor = ledManager->getCurrentLedColor(swPort);
  EXPECT_EQ(offLedColor, led::LedColor::OFF);

  // Verify link Up, the expected LED color is either Blue or Green
  ledManager->setExternalLedState(
      swPort, PortLedExternalState::EXTERNAL_FORCE_ON);
  auto onLedColorCurrent = ledManager->getCurrentLedColor(swPort);
  auto onLedColorExpected = ledManager->onColor();
  EXPECT_EQ(onLedColorCurrent, onLedColorExpected);

  // Put it back to Off state and check again
  ledManager->setExternalLedState(
      swPort, PortLedExternalState::EXTERNAL_FORCE_OFF);
  offLedColor = ledManager->getCurrentLedColor(swPort);
  EXPECT_EQ(offLedColor, led::LedColor::OFF);
}

} // namespace facebook::fboss

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);

  facebook::initFacebook(&argc, &argv);

  return RUN_ALL_TESTS();
}
