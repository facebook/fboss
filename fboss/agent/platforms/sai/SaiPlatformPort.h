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
  void linkSpeedChanged(const cfg::PortSpeed& speed) override;
  bool supportsTransceiver() const override;
  std::optional<TransceiverID> getTransceiverID() const override;
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
  virtual std::vector<PortID> getSubsumedPorts(cfg::PortSpeed speed) const;
  virtual phy::FecMode getFecMode(cfg::PortSpeed speed) const;
  virtual TransmitterTechnology getTransmitterTech();

 private:
  std::optional<cfg::PlatformPortSettings> getPlatformPortSettings(
      cfg::PortSpeed speed) const;
};

} // namespace facebook::fboss
