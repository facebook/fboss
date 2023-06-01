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

class BcmTestMinipackPlatform;

class BcmTestMinipackPort : public BcmTestPort {
 public:
  BcmTestMinipackPort(PortID id, BcmTestMinipackPlatform* platform);

  LaneSpeeds supportedLaneSpeeds() const override {
    // TODO(joseph5wu) We haven't support flexport and portgroup for TH3
    return LaneSpeeds();
  }

  bool shouldUsePortResourceAPIs() const override {
    return true;
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestMinipackPort(BcmTestMinipackPort const&) = delete;
  BcmTestMinipackPort& operator=(BcmTestMinipackPort const&) = delete;
};

} // namespace facebook::fboss
