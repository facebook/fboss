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

#include "fboss/agent/platforms/test_platforms/BcmTestWedgePlatform.h"

namespace facebook {
namespace fboss {
class BcmTestWedge40Platform : public BcmTestWedgePlatform {
 public:
  BcmTestWedge40Platform();
  ~BcmTestWedge40Platform() override {}

  bool isCosSupported() const override {
    return false;
  }

  bool v6MirrorTunnelSupported() const override {
    return false;
  }
  bool sflowSamplingSupported() const override {
    return false;
  }

  std::list<FlexPortMode> getSupportedFlexPortModes() const override {
    return {FlexPortMode::ONEX40G, FlexPortMode::FOURX10G};
  }
  const PortQueue& getDefaultPortQueueSettings(
      cfg::StreamType streamType) const override;
  const PortQueue& getDefaultControlPlaneQueueSettings(
      cfg::StreamType streamType) const override;

  uint32_t getMMUBufferBytes() const override {
    return 16 * 1024 * 1024;
  }
  uint32_t getMMUCellBytes() const override {
    return 208;
  }

  bool useQueueGportForCos() const override {
    return false;
  }

  uint32_t maxLabelStackDepth() const override {
    return 2;
  }

  bool isMultiPathLabelSwitchActionSupported() const override {
    return false;
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestWedge40Platform(BcmTestWedge40Platform const&) = delete;
  BcmTestWedge40Platform& operator=(BcmTestWedge40Platform const&) = delete;

  std::unique_ptr<BcmTestPort> createTestPort(PortID id) const override;
};
} // namespace fboss
} // namespace facebook
