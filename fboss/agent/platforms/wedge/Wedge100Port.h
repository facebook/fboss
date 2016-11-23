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

#include "fboss/agent/platforms/wedge/WedgePort.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook { namespace fboss {

class Wedge100Platform;

enum class LedColor : uint32_t {
  OFF = 0b000,
  BLUE = 0b001,
  GREEN = 0b010,
  CYAN = 0b011,
  RED = 0b100,
  MAGENTA = 0b101,
  YELLOW = 0b110,
  WHITE = 0b111,
};

class Wedge100Port : public WedgePort {
 public:
  explicit Wedge100Port(PortID id, Wedge100Platform* platform,
                        TransceiverID frontPanelPort,
                        ChannelID channel)
      : WedgePort(id, frontPanelPort, channel),
        platform_(platform) {}

  LaneSpeeds supportedLaneSpeeds() const override {
    LaneSpeeds speeds;
    speeds.insert(cfg::PortSpeed::GIGE);
    speeds.insert(cfg::PortSpeed::XG);
    speeds.insert(cfg::PortSpeed::TWENTYFIVEG);
    return speeds;
  }

  void remedy() override;
  void prepareForGracefulExit() override;
  void linkStatusChanged(bool up, bool adminUp) override;
  bool shouldDisableFEC() const override {
    return false;
  }

 private:
  bool isTop() {
    return !(frontPanelPort_ & 0x1);
  }

  BcmPortGroup::LaneMode laneMode();
  bool useCompactMode();
  LedColor getLedColor(bool up, bool adminUp);

  Wedge100Platform* platform_{nullptr};
};

}} // facebook::fboss
