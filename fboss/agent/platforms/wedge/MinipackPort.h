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

namespace facebook { namespace fboss {

class MinipackPlatform;

class MinipackPort : public WedgePort {
public:
  MinipackPort(PortID id,
               WedgePlatform* platform,
               folly::Optional<TransceiverID> frontPanelPort,
               folly::Optional<ChannelID> channel) :
    WedgePort(id, platform, frontPanelPort, channel) {}

  LaneSpeeds supportedLaneSpeeds() const override;

  void prepareForGracefulExit() override;
  void linkStatusChanged(bool up, bool adminUp) override;
  bool shouldDisableFEC() const override;

  const XPEs getEgressXPEs() const override;
};

}} // facebook::fboss
