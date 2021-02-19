// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

#include "fboss/lib/fpga/MinipackSystemContainer.h"
#include "fboss/lib/fpga/facebook/tests/FakeFpgaDevice.h"
#include "fboss/lib/fpga/tests/BaseSystemContainerTests.h"

namespace {
constexpr auto kFakePhysicalAddr = 0xfdf00000;
constexpr auto kFakeSize = 1024 * 1024;
constexpr auto kPimOffset = 0x40000;
constexpr auto kPimStride = 0x8000;
constexpr auto kLedRegisterOffset = 0x0310;
constexpr auto kLedRegisterStride = 0x4;
constexpr auto kPimStart = 2;
constexpr auto kNumPims = 8;
constexpr auto kNumLeds = 16;
} // namespace

namespace facebook::fboss {

class MinipackSystemContainerTests : public BaseSystemContainerTests {
 protected:
  std::unique_ptr<MinipackSystemContainer> systemContainer_;

  void SetUp() override {
    // Use fake fpga device

    systemContainer_ = std::make_unique<MinipackSystemContainer>(
        std::make_unique<FakeFpgaDevice>(kFakePhysicalAddr, kFakeSize));

    // Hold a reference for validation. memory regions will write to the fake
    // device
    fpgaDevice_ =
        dynamic_cast<FakeFpgaDevice*>(systemContainer_->getFpgaDevice());
  }

  uint32_t getLedAddress(int pim, int led) const override {
    auto pimOffset = kPimOffset + kPimStride * (pim - 2);
    return pimOffset + kLedRegisterOffset + kLedRegisterStride * led;
  }
};

TEST_F(MinipackSystemContainerTests, TestLeds) {
  testPimLeds(
      systemContainer_.get(),
      kNumPims,
      kPimStart,
      kNumLeds,
      MinipackLed::Color::BLUE,
      MinipackLed::Color::OFF);
}

} // namespace facebook::fboss
