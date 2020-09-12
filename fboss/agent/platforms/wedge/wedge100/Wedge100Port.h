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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/platforms/wedge/WedgePort.h"

#include "fboss/agent/platforms/common/utils/Wedge100LedUtils.h"

namespace facebook::fboss {

class Wedge100Platform;

class Wedge100Port : public WedgePort {
 public:
  Wedge100Port(
      PortID id,
      Wedge100Platform* platform,
      std::optional<FrontPanelResources> frontPanel);

  LaneSpeeds supportedLaneSpeeds() const override {
    LaneSpeeds speeds;
    speeds.insert(cfg::PortSpeed::GIGE);
    speeds.insert(cfg::PortSpeed::XG);
    speeds.insert(cfg::PortSpeed::TWENTYFIVEG);
    return speeds;
  }

  void prepareForGracefulExit() override;
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState) override;
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

  std::vector<phy::PinConfig> getIphyPinConfigs(
      cfg::PortProfileID profileID) const override;

 private:
  bool isTop();

  bool useCompactMode();
  Wedge100LedUtils::LedColor getLedColor(bool up, bool adminUp);
  Wedge100LedUtils::LedColor internalLedState_;
  void setPortLedColorMode(Wedge100LedUtils::LedColor);
};

} // namespace facebook::fboss
