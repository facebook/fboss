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

namespace facebook::fboss {

class GalaxyPlatform;

class GalaxyPort : public WedgePort {
 public:
  GalaxyPort(
      PortID id,
      GalaxyPlatform* platform,
      std::optional<FrontPanelResources> frontPanel);

  LaneSpeeds supportedLaneSpeeds() const override {
    LaneSpeeds speeds;
    speeds.insert(cfg::PortSpeed::GIGE);
    speeds.insert(cfg::PortSpeed::XG);
    speeds.insert(cfg::PortSpeed::TWENTYFIVEG);
    return speeds;
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

  void prepareForGracefulExit() override {}
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState) override;

  bool isBackplanePort() const {
    return !(getTransceiverID().has_value());
  }
  bool shouldDisableFEC() const override {
    // Only disable FEC if this is a backplane port
    return isBackplanePort();
  }
};

} // namespace facebook::fboss
