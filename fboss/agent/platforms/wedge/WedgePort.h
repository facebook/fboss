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

#include <folly/futures/Future.h>
#include <optional>

#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmPlatformPort.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/Port.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook::fboss {

class WedgePlatform;

struct FrontPanelResources {
  FrontPanelResources(
      TransceiverID transceiver,
      std::vector<ChannelID> channels)
      : transceiver(transceiver), channels(std::move(channels)) {}

  TransceiverID transceiver;
  std::vector<ChannelID> channels;
};

class WedgePort : public BcmPlatformPort {
 protected:
  WedgePort(PortID id, WedgePlatform* platform);

  // TODO(joseph5wu) Will deprecate this constructor demanding front panel
  WedgePort(
      PortID id,
      WedgePlatform* platform,
      std::optional<FrontPanelResources> frontPanel);

 public:
  void setBcmPort(BcmPort* port) override;
  BcmPort* getBcmPort() const override {
    return bcmPort_;
  }

  void preDisable(bool temporary) override;
  void postDisable(bool temporary) override;
  void preEnable() override;
  void postEnable() override;
  bool isMediaPresent() override;
  void statusIndication(
      bool enabled,
      bool link,
      bool ingress,
      bool egress,
      bool discards,
      bool errors) override;
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState) override;

  PortStatus toThrift(const std::shared_ptr<Port>& port);

  std::optional<TransceiverID> getTransceiverID() const override {
    return transceiverID_;
  }

  bool supportsTransceiver() const override;

  // TODO: deprecate this
  virtual std::optional<ChannelID> getChannel() const;

  std::vector<int32_t> getChannels() const;

  TransceiverIdxThrift getTransceiverMapping() const;

  folly::Future<TransmitterTechnology> getTransmitterTech(
      folly::EventBase* evb) const override;
  folly::Future<std::optional<Cable>> getCableInfo(folly::EventBase* evb) const;
  folly::Future<std::optional<ExtendedSpecComplianceCode>>
  getTransceiverExtendedSpecCompliance(folly::EventBase* evb) const;

  bool shouldUsePortResourceAPIs() const override {
    return false;
  }
  bool shouldSetupPortGroup() const override {
    return true;
  }

  virtual void portChanged(std::shared_ptr<Port> port) {
    port_ = std::move(port);
  }

  BcmPortGroup::LaneMode getLaneMode() const;

  folly::Future<TransceiverInfo> getTransceiverInfo() const override;

 protected:
  bool isControllingPort() const;

  // TODO(joseph5wu): deprecate front panel resource
  std::optional<FrontPanelResources> frontPanel_;

  BcmPort* bcmPort_{nullptr};
  std::shared_ptr<Port> port_{nullptr};

 private:
  // Forbidden copy constructor and assignment operator
  WedgePort(WedgePort const&) = delete;
  WedgePort& operator=(WedgePort const&) = delete;

  // Get Transceiver lanes from config
  // Return std::nullopt if there's no platform config
  // Return empty vector if the port doesn't support transceivers
  // Return non-empty vector to maintain the lanes of the transceivers
  std::optional<std::vector<phy::PinID>> getTransceiverLanes(
      std::optional<cfg::PortProfileID> profileID = std::nullopt) const;

  // transceiver id should be fixed once the PlatformPort is created
  std::optional<TransceiverID> transceiverID_;
};

} // namespace facebook::fboss
