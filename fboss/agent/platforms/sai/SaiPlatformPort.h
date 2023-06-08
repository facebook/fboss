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
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Transceiver.h"
#include "fboss/agent/types.h"

#include <optional>

DECLARE_bool(skip_transceiver_programming);
DECLARE_bool(skip_led_programming);

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
  virtual uint32_t getPhysicalLaneId(uint32_t chipId, uint32_t logicalLane)
      const = 0;
  bool checkSupportsTransceiver() const;
  TransceiverIdxThrift getTransceiverMapping(cfg::PortSpeed speed);

  folly::Future<TransceiverInfo> getFutureTransceiverInfo() const override;
  std::shared_ptr<TransceiverSpec> getTransceiverSpec() const override;

  void setCurrentProfile(cfg::PortProfileID profile) {
    profile_ = profile;
  }
  cfg::PortProfileID getCurrentProfile() const {
    return profile_;
  }
  virtual void portChanged(
      std::shared_ptr<Port> newPort,
      std::shared_ptr<Port> oldPort) = 0;

  int getLaneCount() const;
  std::optional<ChannelID> getChannel() const;
  virtual uint32_t getCurrentLedState() const = 0;

 private:
  cfg::PortProfileID profile_{};
};

} // namespace facebook::fboss
