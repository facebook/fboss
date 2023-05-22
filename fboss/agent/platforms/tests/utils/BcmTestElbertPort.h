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

class BcmTestElbertPlatform;

class BcmTestElbertPort : public BcmTestPort {
 public:
  BcmTestElbertPort(PortID id, BcmTestElbertPlatform* platform);

  LaneSpeeds supportedLaneSpeeds() const override {
    return LaneSpeeds();
  }

  bool shouldUsePortResourceAPIs() const override {
    return true;
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestElbertPort(BcmTestElbertPort const&) = delete;
  BcmTestElbertPort& operator=(BcmTestElbertPort const&) = delete;
};

} // namespace facebook::fboss
