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

#include "fboss/agent/platforms/tests/utils/BcmTestWedgeTomahawkPlatform.h"

namespace facebook::fboss {

class BcmTestGalaxyPlatform : public BcmTestWedgeTomahawkPlatform {
 public:
  using BcmTestWedgeTomahawkPlatform::BcmTestWedgeTomahawkPlatform;

  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {FlexPortMode::ONEX100G};
  }

  void initLEDs(int unit) override;

  bool verifyLEDStatus(PortID port, bool up) override;

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestGalaxyPlatform(BcmTestGalaxyPlatform const&) = delete;
  BcmTestGalaxyPlatform& operator=(BcmTestGalaxyPlatform const&) = delete;

  std::unique_ptr<BcmTestPort> createTestPort(PortID id) override;
};

class BcmTestGalaxyLCPlatform : public BcmTestGalaxyPlatform {
 public:
  explicit BcmTestGalaxyLCPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo);
  ~BcmTestGalaxyLCPlatform() override {}

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestGalaxyLCPlatform(BcmTestGalaxyLCPlatform const&) = delete;
  BcmTestGalaxyLCPlatform& operator=(BcmTestGalaxyLCPlatform const&) = delete;
};

class BcmTestGalaxyFCPlatform : public BcmTestGalaxyPlatform {
 public:
  explicit BcmTestGalaxyFCPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo);
  ~BcmTestGalaxyFCPlatform() override {}

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestGalaxyFCPlatform(BcmTestGalaxyFCPlatform const&) = delete;
  BcmTestGalaxyFCPlatform& operator=(BcmTestGalaxyFCPlatform const&) = delete;
};
} // namespace facebook::fboss
