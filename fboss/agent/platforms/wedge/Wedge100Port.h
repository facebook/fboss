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

#include "fboss/agent/FbossError.h"
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
  Wedge100Port(PortID id,
               WedgePlatform* platform,
               folly::Optional<FrontPanelResources> frontPanel) :
      WedgePort(id, platform, std::move(frontPanel)) {}

  LaneSpeeds supportedLaneSpeeds() const override {
    LaneSpeeds speeds;
    speeds.insert(cfg::PortSpeed::GIGE);
    speeds.insert(cfg::PortSpeed::XG);
    speeds.insert(cfg::PortSpeed::TWENTYFIVEG);
    return speeds;
  }

  void prepareForGracefulExit() override;
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PlatformPort::ExternalState) override;
  bool shouldDisableFEC() const override {
    return false;
  }

  const XPEs getEgressXPEs() const override {
    auto pipe = bcmPort_->getPipe();
    if (pipe == 0 || pipe == 1) {
      return {0, 2};
    } else if (pipe == 2 || pipe == 3) {
      return {1, 3};
    }
    throw FbossError("Unexpected pipe ", pipe);
  }

 private:
  bool isTop() {
    return frontPanel_ ? !(frontPanel_->transceiver & 0x1) : false;
  }

  TxOverrides getTxOverrides() const override;

  BcmPortGroup::LaneMode laneMode();
  bool useCompactMode();
  LedColor getLedColor(bool up, bool adminUp);
};

}} // facebook::fboss
