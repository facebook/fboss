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

#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

#include <optional>

namespace facebook::fboss {

class SaiPlatform;

class SaiPlatformPort : public PlatformPort {
 public:
  SaiPlatformPort(PortID id, SaiPlatform* platform);
  void preDisable(bool temporary) override;
  void postDisable(bool temporary) override;
  void preEnable() override;
  void postEnable() override;
  bool isMediaPresent() override;
  void linkStatusChanged(bool up, bool adminUp) override;
  std::optional<TransceiverID> getTransceiverID() const override {
    return transceiverID_;
  }
  void statusIndication(
      bool enabled,
      bool link,
      bool ingress,
      bool egress,
      bool discards,
      bool errors) override;
  void prepareForGracefulExit() override;
  bool shouldDisableFEC() const override;
  void externalState(PortLedExternalState) override {}
  virtual std::vector<uint32_t> getHwPortLanes(cfg::PortSpeed speed) const;
  virtual std::vector<uint32_t> getHwPortLanes(
      cfg::PortProfileID profileID) const;
  virtual std::vector<PortID> getSubsumedPorts(cfg::PortSpeed speed) const;
  virtual TransmitterTechnology getTransmitterTech();
  virtual uint32_t getPhysicalLaneId(uint32_t chipId, uint32_t logicalLane)
      const = 0;
  bool checkSupportsTransceiver() const;
  TransceiverIdxThrift getTransceiverMapping(cfg::PortSpeed speed);

  std::optional<Cable> getCableInfo() const;
  std::vector<phy::PinConfig> getIphyPinConfigs(
      cfg::PortProfileID profileID) const override;

  folly::Future<TransceiverInfo> getTransceiverInfo() const override;

  void setCurrentProfile(cfg::PortProfileID profile) {
    profile_ = profile;
  }
  cfg::PortProfileID getCurrentProfile() const {
    return profile_;
  }
  int getLaneCount() const;
  std::optional<ChannelID> getChannel() const;
  virtual uint32_t getCurrentLedState() const = 0;

 private:
  std::vector<phy::PinID> getTransceiverLanes() const;
  folly::Future<TransmitterTechnology> getTransmitterTechInternal(
      folly::EventBase* evb);
  folly::Future<std::optional<Cable>> getCableInfoInternal(
      folly::EventBase* evb) const;
  std::optional<TransceiverID> transceiverID_;
  cfg::PortProfileID profile_{};
};

} // namespace facebook::fboss
