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

#include <boost/container/flat_map.hpp>

#include <folly/futures/Future.h>
#include <optional>

#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmPlatformPort.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Transceiver.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook::fboss {

class WedgePlatform;

class WedgePort : public BcmPlatformPort {
 protected:
  WedgePort(PortID id, WedgePlatform* platform);

 public:
  void setBcmPort(BcmPort* port) override;
  BcmPort* getBcmPort() const override {
    return bcmPort_;
  }

  void preDisable(bool temporary) override;
  void postDisable(bool temporary) override;
  void preEnable() override;
  void postEnable() override;
  bool isMediaPresent() override;
  void statusIndication(
      bool enabled,
      bool link,
      bool ingress,
      bool egress,
      bool discards,
      bool errors) override;
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState) override;

  PortStatus toThrift(const std::shared_ptr<Port>& port);

  bool supportsTransceiver() const override;

  // TODO: deprecate this
  virtual std::optional<ChannelID> getChannel() const;

  std::vector<int32_t> getChannels() const;

  TransceiverIdxThrift getTransceiverMapping() const;

  bool shouldUsePortResourceAPIs() const override {
    return false;
  }

  virtual void portChanged(std::shared_ptr<Port> port) {
    port_ = std::move(port);
  }

  BcmPortGroup::LaneMode getLaneMode() const;

  std::shared_ptr<TransceiverSpec> getTransceiverSpec() const override;

 protected:
  bool isControllingPort() const;

  BcmPort* bcmPort_{nullptr};
  std::shared_ptr<Port> port_{nullptr};

 private:
  // Forbidden copy constructor and assignment operator
  WedgePort(WedgePort const&) = delete;
  WedgePort& operator=(WedgePort const&) = delete;
};

} // namespace facebook::fboss
