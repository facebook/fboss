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

#include "fboss/agent/hw/bcm/tests/platforms/BcmTestPort.h"

namespace facebook {
namespace fboss {
class BcmTestGalaxyPort : public BcmTestPort {
 public:
  explicit BcmTestGalaxyPort(PortID id);

  LaneSpeeds supportedLaneSpeeds() const override {
    return {cfg::PortSpeed::TWENTYFIVEG};
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestGalaxyPort(BcmTestGalaxyPort const&) = delete;
  BcmTestGalaxyPort& operator=(BcmTestGalaxyPort const&) = delete;
};
} // namespace fboss
} // namespace facebook
