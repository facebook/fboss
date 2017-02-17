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
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook { namespace fboss {


class GalaxyPort : public WedgePort {
 public:
  GalaxyPort(PortID id, folly::Optional<TransceiverID> frontPanelPort,
      folly::Optional<ChannelID> channel,
      bool isBackplanePort)
      : WedgePort(id, frontPanelPort, channel),
        isBackplanePort_(isBackplanePort) {}

  LaneSpeeds supportedLaneSpeeds() const override {
    LaneSpeeds speeds;
    speeds.insert(cfg::PortSpeed::GIGE);
    speeds.insert(cfg::PortSpeed::XG);
    speeds.insert(cfg::PortSpeed::TWENTYFIVEG);
    return speeds;
  }

  void remedy() override {}
  void prepareForGracefulExit() override {}
  void linkStatusChanged(bool up, bool adminUp) override;
  bool shouldDisableFEC() const override {
    return isBackplanePort_;
  }

 private:
  /* Its useful in certain contexts
   * to know whether this is a bacplane
   * port or not. For backplane ports, since
   * there is no transceiver, the settings
   * needed might be different from a front
   * panel port
   */
  const bool isBackplanePort_{false};
};

}} // facebook::fboss
