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

namespace facebook::fboss {

class Wedge40Platform;

class Wedge40Port : public WedgePort {
 public:
  Wedge40Port(
      PortID id,
      Wedge40Platform* platform,
      std::optional<FrontPanelResources> frontPanel);

  LaneSpeeds supportedLaneSpeeds() const override {
    LaneSpeeds speeds;
    speeds.insert(cfg::PortSpeed::GIGE);
    speeds.insert(cfg::PortSpeed::XG);
    return speeds;
  }

  const XPEs getEgressXPEs() const override {
    return {0};
  }

  void remedy();
  void prepareForGracefulExit() override;
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState) override;
  bool shouldDisableFEC() const override {
    return false;
  }
};

} // namespace facebook::fboss
