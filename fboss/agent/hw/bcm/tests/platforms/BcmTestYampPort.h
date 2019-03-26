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

namespace facebook { namespace fboss {
class BcmTestYampPort : public BcmTestPort {
public:
  explicit BcmTestYampPort(PortID id) : BcmTestPort(id) {}

  LaneSpeeds supportedLaneSpeeds() const override {
    // TODO(joseph5wu) We haven't support flexport and portgroup for TH3
    return {};
  }

  bool shouldUsePortResourceAPIs() const override {
    return true;
  }
  bool shouldSetupPortGroup() const override {
    return false;
  }

private:
  // Forbidden copy constructor and assignment operator
  BcmTestYampPort(BcmTestYampPort const&) = delete;
  BcmTestYampPort& operator=(BcmTestYampPort const&) = delete;
};
}} // facebook::fboss
