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

#include <boost/container/flat_map.hpp>

#include "fboss/agent/hw/bcm/BcmPlatformPort.h"

namespace facebook::fboss {

class BcmTestPlatform;

class BcmTestPort : public BcmPlatformPort {
 public:
  BcmTestPort(PortID id, BcmTestPlatform* platform);
  void setBcmPort(BcmPort* port) override;
  BcmPort* getBcmPort() const override {
    return bcmPort_;
  }

  virtual LaneSpeeds supportedLaneSpeeds() const override = 0;

  void preDisable(bool temporary) override;
  void postDisable(bool temporary) override;
  void preEnable() override;
  void postEnable() override;
  bool isMediaPresent() override;
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState) override;
  bool supportsTransceiver() const override;
  void statusIndication(
      bool enabled,
      bool link,
      bool ingress,
      bool egress,
      bool discards,
      bool errors) override;
  void prepareForGracefulExit() override;
  void setShouldDisableFec() {
    shouldDisableFec_ = true;
  }
  bool shouldDisableFEC() const override {
    return shouldDisableFec_;
  }

  const XPEs getEgressXPEs() const override {
    return {0};
  }

  bool shouldUsePortResourceAPIs() const override {
    return false;
  }

  folly::Future<TransceiverInfo> getFutureTransceiverInfo() const override;
  std::shared_ptr<TransceiverSpec> getTransceiverSpec() const override;

  int numberOfLanes() const;

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestPort(BcmTestPort const&) = delete;
  BcmTestPort& operator=(BcmTestPort const&) = delete;

  bool shouldDisableFec_{false};
  BcmPort* bcmPort_{nullptr};
};

} // namespace facebook::fboss
