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

DEFINE_int32(visual_delay_sec, 0, "Add a delay to enable seeing LED change");

void LedServiceTest::SetUp() {
  // Create ensemble and initialize it
  ensemble_ = std::make_unique<LedEnsemble>();
  ensemble_->init();

  // Check if the LED manager is created correctly, the config file is loaded
  // and the LED is managed by service now
  ledManager_ = getLedEnsemble()->getLedManager();
  CHECK_NE(ledManager_, nullptr);
  CHECK(ledManager_->isLedControlledThroughService());
  platformMap_ = ledManager_->getPlatformMapping();
  CHECK_NE(platformMap_, nullptr);
}

void LedServiceTest::TearDown() {
  // Remove ensemble and objects contained there
  ensemble_.reset();
  ledManager_ = nullptr;
  platformMap_ = nullptr;
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
TEST_F(LedServiceTest, checkForceLed) {
  // Test for all the modules
  // Use the ports max speed and profile
  auto transceivers = getAllTransceivers(platformMap_);
  std::sort(transceivers.begin(), transceivers.end());

  for (auto tcvr : transceivers) {
    auto swPorts = platformMap_->getSwPortListFromTransceiverId(tcvr);
    CHECK_GT(swPorts.size(), 0);
    auto swPort = swPorts[0];
    auto swPortName = platformMap_->getPortNameByPortId(swPort);
    CHECK(swPortName.has_value());

    // The setExternalLedState will throw because first update from FSDB has not
    // happened to the LedManager
    EXPECT_THROW(
        ledManager_->setExternalLedState(
            swPort, PortLedExternalState::EXTERNAL_FORCE_OFF),
        FbossError);

    // Do the first update from FSDB to LedService
    auto maxSpeed = platformMap_->getPortMaxSpeed(swPort);
    auto profile = platformMap_->getProfileIDBySpeed(swPort, maxSpeed);
    LedManager::LedSwitchStateUpdate ledUpdate = {
        static_cast<short>(swPort),
        "",
        enumToName<cfg::PortProfileID>(profile),
        false};

    std::map<short, LedManager::LedSwitchStateUpdate> switchUpdate_0;
    switchUpdate_0[swPort] = ledUpdate;
    ledManager_->updateLedStatus(switchUpdate_0);

    // Now the setting of external LED state should be successful
    EXPECT_NO_THROW(ledManager_->setExternalLedState(
        swPort, PortLedExternalState::EXTERNAL_FORCE_OFF));

    // Verify link Down, the expected LED color is OFF
    auto offLedColor = ledManager_->getCurrentLedColor(swPort);
    auto ledState = ledManager_->getPortLedState(swPortName.value());
    EXPECT_EQ(offLedColor, led::LedColor::OFF);
    EXPECT_EQ(
        ledState.currentLedState()->ledColor().value(), led::LedColor::OFF);
    EXPECT_TRUE(ledState.forcedOffState().value());

    // Verify forcing LED to on state that LED is set accordingly.
    ledManager_->setExternalLedState(
        swPort, PortLedExternalState::EXTERNAL_FORCE_ON);
    auto onLedColorCurrent = ledManager_->getCurrentLedColor(swPort);
    auto onLedColorExpected = ledManager_->forcedOnColor();
    ledState = ledManager_->getPortLedState(swPortName.value());
    EXPECT_EQ(onLedColorCurrent, onLedColorExpected);
    EXPECT_EQ(
        ledState.currentLedState()->ledColor().value(), onLedColorExpected);
    EXPECT_TRUE(ledState.forcedOnState().value());

    // If the flags is specified, add a delay before forcing LED off
    // to enable seeing the LED change
    sleep(FLAGS_visual_delay_sec);

    // Put it back to Off state and check again
    ledManager_->setExternalLedState(
        swPort, PortLedExternalState::EXTERNAL_FORCE_OFF);
    offLedColor = ledManager_->getCurrentLedColor(swPort);
    ledState = ledManager_->getPortLedState(swPortName.value());
    EXPECT_EQ(offLedColor, led::LedColor::OFF);
    EXPECT_EQ(
        ledState.currentLedState()->ledColor().value(), led::LedColor::OFF);
    EXPECT_TRUE(ledState.forcedOffState().value());
  }
}

void LedServiceTest::checkLedColor(
    PortID port,
    enum led::LedColor color,
    int testNum) {
  auto portName = platformMap_->getPortNameByPortId(port);
  CHECK(portName.has_value());
  auto currentColor = ledManager_->getCurrentLedColor(port);
  auto ledState = ledManager_->getPortLedState(portName.value());
  EXPECT_EQ(currentColor, color)
      << "LED color should be " << enumToName<led::LedColor>(color)
      << " for port " << portName.value() << " but current color is "
      << enumToName<led::LedColor>(currentColor) << " for test " << testNum;
  EXPECT_EQ(ledState.currentLedState()->ledColor().value(), color)
      << "LED state should be " << enumToName<led::LedColor>(color)
      << " for port " << portName.value() << " for test " << testNum;
  // If the flags is specified, add a delay to enable seeing the LED change
  sleep(FLAGS_visual_delay_sec);
}

TEST_F(LedServiceTest, checkLedColorChange) {
  auto transceivers = getAllTransceivers(platformMap_);
  std::sort(transceivers.begin(), transceivers.end());

  for (auto tcvr : transceivers) {
    XLOG(INFO) << "Testing transceiver " << tcvr;
    auto swPorts = platformMap_->getSwPortListFromTransceiverId(tcvr);
    for (auto& swPort : swPorts) {
      // Do the first update from FSDB to LedService. LED set to inactive.
      auto maxSpeed = platformMap_->getPortMaxSpeed(swPort);
      auto profile = platformMap_->getProfileIDBySpeed(swPort, maxSpeed);
      XLOG(INFO) << "Testing SW Port " << swPort << " with speed "
                 << enumToName<cfg::PortSpeed>(maxSpeed) << " and profile "
                 << enumToName<cfg::PortProfileID>(profile);
      LedManager::LedSwitchStateUpdate ledUpdate = {
          static_cast<short>(swPort),
          "",
          enumToName<cfg::PortProfileID>(profile),
          false /*operationalState*/,
          PortLedExternalState::NONE};

      std::map<short, LedManager::LedSwitchStateUpdate> updateMap;
      updateMap[swPort] = ledUpdate;
      ledManager_->updateLedStatus(updateMap);

      auto swPortName = platformMap_->getPortNameByPortId(swPort);
      CHECK(swPortName.has_value());

      int testNum = 0;
      // 1- Verify Default creation with operational state = false that the
      // LED color is off.
      checkLedColor(swPort, led::LedColor::OFF, ++testNum);

      // 2- Set the operational state to true, verify that the LED color
      // is not off anymore. Platform determines its own on color.
      updateMap[swPort].operState = true;
      ledManager_->updateLedStatus(updateMap);
      auto colorBefore = ledManager_->getCurrentLedColor(swPort);
      EXPECT_NE(colorBefore, led::LedColor::OFF);
      checkLedColor(swPort, colorBefore, ++testNum);

      // If the flags is specified, add a delay before forcing LED off
      // to enable visual test for the LED change
      sleep(FLAGS_visual_delay_sec);

      // 3- Force LED to be off. The LED color should be off.
      ledManager_->setExternalLedState(
          swPort, PortLedExternalState::EXTERNAL_FORCE_OFF);
      checkLedColor(swPort, led::LedColor::OFF, ++testNum);

      // 4- Unforce LED Off. LED should go back to the previous color.
      ledManager_->setExternalLedState(swPort, PortLedExternalState::NONE);
      checkLedColor(swPort, colorBefore, ++testNum);

      // 5- Force LED to be on. The LED color should not be off.
      // Each platform determines its own on color.
      ledManager_->setExternalLedState(
          swPort, PortLedExternalState::EXTERNAL_FORCE_ON);
      auto onLedColorExpected = ledManager_->forcedOnColor();
      checkLedColor(swPort, onLedColorExpected, ++testNum);

      // 6- Unforce LED On. LED should go back to the previous color.
      ledManager_->setExternalLedState(swPort, PortLedExternalState::NONE);
      checkLedColor(swPort, colorBefore, ++testNum);

      // 7- Set the cabling error, expect yellow LED.
      updateMap[swPort].ledExternalState = PortLedExternalState::CABLING_ERROR;
      ledManager_->updateLedStatus(updateMap);
      checkLedColor(swPort, led::LedColor::YELLOW, ++testNum);

      // 8- Remove the cabling error, check the LED color goes back
      updateMap[swPort].ledExternalState = PortLedExternalState::NONE;
      ledManager_->updateLedStatus(updateMap);
      checkLedColor(swPort, colorBefore, ++testNum);

      // 9- Set the cabling error, force LED ON, check the LED color is on, and
      // not yellow.
      updateMap[swPort].ledExternalState = PortLedExternalState::CABLING_ERROR;
      ledManager_->updateLedStatus(updateMap);
      ledManager_->setExternalLedState(
          swPort, PortLedExternalState::EXTERNAL_FORCE_ON);
      checkLedColor(swPort, onLedColorExpected, ++testNum);

      // 10 - Remove force on, LED should go back to cabling error.
      ledManager_->setExternalLedState(swPort, PortLedExternalState::NONE);
      checkLedColor(swPort, led::LedColor::YELLOW, ++testNum);

      // 11- Force LED Off.
      ledManager_->setExternalLedState(
          swPort, PortLedExternalState::EXTERNAL_FORCE_OFF);
      checkLedColor(swPort, led::LedColor::OFF, ++testNum);

      // 12 - Remove force off, LED should go back to cabling error.
      ledManager_->setExternalLedState(swPort, PortLedExternalState::NONE);
      checkLedColor(swPort, led::LedColor::YELLOW, ++testNum);

      // 13 - Remove Cabling Error, LED should go back to original color.
      updateMap[swPort].ledExternalState = PortLedExternalState::NONE;
      ledManager_->updateLedStatus(updateMap);
      checkLedColor(swPort, colorBefore, ++testNum);

      // 14 - Force LED Off.
      ledManager_->setExternalLedState(
          swPort, PortLedExternalState::EXTERNAL_FORCE_OFF);
      checkLedColor(swPort, led::LedColor::OFF, ++testNum);

      // 15 - Remove Force Off. LED should be in original color.
      ledManager_->setExternalLedState(swPort, PortLedExternalState::NONE);
      checkLedColor(swPort, colorBefore, ++testNum);

      // 16 - set Operational state off. Expect LED to be off.
      updateMap[swPort].operState = false;
      ledManager_->updateLedStatus(updateMap);
      checkLedColor(swPort, led::LedColor::OFF, ++testNum);
    }
  }
}

} // namespace facebook::fboss

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);

  facebook::initFacebook(&argc, &argv);

  return RUN_ALL_TESTS();
}
