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

#include "fboss/agent/platforms/test_platforms/BcmTestWedgeTomahawkPlatform.h"

namespace facebook {
namespace fboss {
class BcmTestGalaxyPlatform : public BcmTestWedgeTomahawkPlatform {
 public:
  BcmTestGalaxyPlatform();
  ~BcmTestGalaxyPlatform() override {}

  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {FlexPortMode::ONEX100G};
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestGalaxyPlatform(BcmTestGalaxyPlatform const&) = delete;
  BcmTestGalaxyPlatform& operator=(BcmTestGalaxyPlatform const&) = delete;

  std::unique_ptr<BcmTestPort> createTestPort(PortID id) override;
};
} // namespace fboss
} // namespace facebook
