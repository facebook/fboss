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

#include "fboss/agent/platforms/tests/utils/BcmTestPort.h"

namespace facebook::fboss {

class BcmTestWedge400Platform;

class BcmTestWedge400Port : public BcmTestPort {
 public:
  BcmTestWedge400Port(PortID id, BcmTestWedge400Platform* platform);

  LaneSpeeds supportedLaneSpeeds() const override {
    // TODO(jennylli) support flexport
    return LaneSpeeds();
  }

  bool shouldUsePortResourceAPIs() const override {
    return true;
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestWedge400Port(BcmTestWedge400Port const&) = delete;
  BcmTestWedge400Port& operator=(BcmTestWedge400Port const&) = delete;
};

} // namespace facebook::fboss
