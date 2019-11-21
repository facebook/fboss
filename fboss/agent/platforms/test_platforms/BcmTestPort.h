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

namespace facebook {
namespace fboss {
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
  void externalState(BcmPlatformPort::ExternalState) override;
  void linkSpeedChanged(const cfg::PortSpeed& speed) override;
  folly::Future<TransmitterTechnology> getTransmitterTech(
      folly::EventBase* evb) const override;
  std::optional<TransceiverID> getTransceiverID() const override;
  folly::Future<std::optional<TxSettings>> getTxSettings(
      folly::EventBase* evb) const override;
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

  bool supportsAddRemovePort() const override {
    return false;
  }
  bool shouldUsePortResourceAPIs() const override {
    return false;
  }

  bool shouldSetupPortGroup() const override {
    return true;
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestPort(BcmTestPort const&) = delete;
  BcmTestPort& operator=(BcmTestPort const&) = delete;

  BcmTestPort::TxOverrides getTxOverrides() const override {
    return BcmTestPort::TxOverrides();
  }

  bool shouldDisableFec_{false};
  BcmPort* bcmPort_{nullptr};
};

} // namespace fboss
} // namespace facebook
