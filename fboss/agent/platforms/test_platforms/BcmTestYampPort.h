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

#include "fboss/agent/platforms/test_platforms/BcmTestPort.h"

namespace facebook {
namespace fboss {
class BcmTestYampPort : public BcmTestPort {
 public:
  explicit BcmTestYampPort(PortID id) : BcmTestPort(id) {}

  LaneSpeeds supportedLaneSpeeds() const override {
    // TODO(joseph5wu) We haven't support flexport and portgroup for TH3
    return LaneSpeeds();
  }

  bool shouldUsePortResourceAPIs() const override {
    return true;
  }
  bool shouldSetupPortGroup() const override {
    return false;
  }

  folly::Future<TransmitterTechnology> getTransmitterTech(
      folly::EventBase* /*evb*/) const override {
    // For Tomahawk3 port, we always use BACKPLANE
    return folly::makeFuture<TransmitterTechnology>(
        TransmitterTechnology::BACKPLANE);
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestYampPort(BcmTestYampPort const&) = delete;
  BcmTestYampPort& operator=(BcmTestYampPort const&) = delete;
};
} // namespace fboss
} // namespace facebook
