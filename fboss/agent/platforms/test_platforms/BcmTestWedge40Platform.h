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

namespace facebook::fboss {

class Trident2Asic;
class PlatformProductInfo;

class BcmTestWedge40Platform : public BcmTestWedgePlatform {
 public:
  explicit BcmTestWedge40Platform(std::unique_ptr<PlatformProductInfo> product);
  ~BcmTestWedge40Platform() override;

  bool isCosSupported() const override {
    return false;
  }

  bool v6MirrorTunnelSupported() const override {
    return false;
  }
  bool sflowSamplingSupported() const override {
    return false;
  }

  bool mirrorPktTruncationSupported() const override {
    return false;
  }

  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
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

  cfg::PortLoopbackMode desiredLoopbackMode() const override {
    // Changing loopback mode to MAC on a 40G port on trident2 changes
    // the speed to 10G unexpectedly.
    //
    // Broadcom case: CS8832244
    //
    return cfg::PortLoopbackMode::PHY;
  }

  HwAsic* getAsic() const override;

  int getDefaultNumPortQueues(cfg::StreamType /* streamType */) const override {
    // Wedge40 doesn't support port queue, return 0
    return 0;
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestWedge40Platform(BcmTestWedge40Platform const&) = delete;
  BcmTestWedge40Platform& operator=(BcmTestWedge40Platform const&) = delete;

  std::unique_ptr<BcmTestPort> createTestPort(PortID id) override;
  std::unique_ptr<Trident2Asic> asic_;
};

} // namespace facebook::fboss
