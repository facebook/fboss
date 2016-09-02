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
#include "fboss/agent/gen-cpp/switch_config_types.h"

namespace facebook { namespace fboss {


class GalaxyPort : public WedgePort {
 public:
  explicit GalaxyPort(PortID id) : WedgePort(id) {}

  LaneSpeeds supportedLaneSpeeds() const override {
    return {
      cfg::PortSpeed::GIGE,
      cfg::PortSpeed::XG,
      cfg::PortSpeed::TWENTYFIVEG
    };
  }

  void remedy() override {}
  void prepareForGracefulExit() override {}
  void linkStatusChanged(bool up, bool adminUp) override;
};

}} // facebook::fboss
