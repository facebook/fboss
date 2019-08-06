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
class BcmTestWedge400DQPort : public BcmTestPort {
 public:
  explicit BcmTestWedge400DQPort(PortID id) : BcmTestPort(id) {}

  LaneSpeeds supportedLaneSpeeds() const override {
    // TODO(jennylli) support flexport
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
    return folly::makeFuture<TransmitterTechnology>(
        TransmitterTechnology::BACKPLANE);
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestWedge400DQPort(BcmTestWedge400DQPort const&) = delete;
  BcmTestWedge400DQPort& operator=(BcmTestWedge400DQPort const&) = delete;
};
} // namespace fboss
} // namespace facebook
