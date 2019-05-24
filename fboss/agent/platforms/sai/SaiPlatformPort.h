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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

#include <folly/Optional.h>

namespace facebook {
namespace fboss {

class SaiPlatformPort : public PlatformPort {
  PortID getPortID() const override;
  void preDisable(bool temporary) override;
  void postDisable(bool temporary) override;
  void preEnable() override;
  void postEnable() override;
  bool isMediaPresent() override;
  void linkStatusChanged(bool up, bool adminUp) override;
  void linkSpeedChanged(const cfg::PortSpeed& speed) override;
  bool supportsTransceiver() const override;
  folly::Optional<TransceiverID> getTransceiverID() const override;
  void statusIndication(
      bool enabled,
      bool link,
      bool ingress,
      bool egress,
      bool discards,
      bool errors) override;
  void prepareForGracefulExit() override;
  bool shouldDisableFEC() const override;
};

} // namespace fboss
} // namespace facebook
