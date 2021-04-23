// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <gtest/gtest.h>

#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/fpga/facebook/tests/FakeFpgaDevice.h"

namespace facebook::fboss {

class BaseSystemContainerTests : public ::testing::Test {
 public:
  template <typename SystemContainer, typename Color>
  void testPimLeds(
      SystemContainer* systemContainer,
      int numPims,
      int pimStart,
      int numLeds,
      Color onColor,
      Color offColor) {
    // TODO(joseph5wu) Will deprecate this function and use the one with
    // map<int, int> for expectedPimToNumLeds
    std::unordered_map<int, int> expectedPimToNumLeds;
    for (auto pim = pimStart; pim < numPims + pimStart; ++pim) {
      expectedPimToNumLeds.emplace(pim, numLeds);
    }
    testPimLeds(
        systemContainer,
        numPims,
        pimStart,
        expectedPimToNumLeds,
        onColor,
        offColor);
  }

  template <typename SystemContainer, typename Color>
  void testPimLeds(
      SystemContainer* systemContainer,
      int numPims,
      int pimStart,
      std::unordered_map<int, int> expectedPimToNumLeds,
      Color onColor,
      Color offColor) {
    for (auto pim = pimStart; pim < numPims + pimStart; pim++) {
      auto pimContainer = systemContainer->getPimContainer(pim);
      for (auto led = 0; led < expectedPimToNumLeds.at(pim); led++) {
        auto ledController = pimContainer->getLedController(led);
        auto ledAddr = getLedAddress(pim, led);
        testLedOnOff(ledController, onColor, offColor, ledAddr);

        // manually set register to yellow and verify reading from fpga
        fpgaDevice_->write(ledAddr, static_cast<uint32_t>(onColor));
        EXPECT_EQ(ledController->getColor(), onColor);
      }
    }
  }

  template <typename SystemContainer>
  void testPimTypes(
      SystemContainer* systemContainer,
      const std::unordered_map<int, MultiPimPlatformPimContainer::PimType>&
          expectedPimToPimType) {
    for (auto pimTypeItr : expectedPimToPimType) {
      EXPECT_EQ(
          systemContainer->getPimType(pimTypeItr.first), pimTypeItr.second);
    }
  }

 protected:
  /*
   This address should be calculated manually, without relying on a
   SystemContainer class, so that we can verify the logic in SystemContainer, in
   case there are changes being made
  */
  virtual uint32_t getLedAddress(int pim, int led) const = 0;
  FakeFpgaDevice* fpgaDevice_;

 private:
  template <typename LedController, typename Color>
  void setAndVerifyLed(
      LedController* ledController,
      Color color,
      uint32_t ledAddress) {
    ledController->setColor(color);
    EXPECT_EQ(ledController->getColor(), color);
    EXPECT_EQ(fpgaDevice_->read(ledAddress), static_cast<uint32_t>(color));
  }

  template <typename LedController, typename Color>
  void testLedOnOff(
      LedController* ledController,
      Color onColor,
      Color offColor,
      uint32_t ledAddress) {
    setAndVerifyLed(ledController, onColor, ledAddress);
    setAndVerifyLed(ledController, offColor, ledAddress);
  }
};

} // namespace facebook::fboss
