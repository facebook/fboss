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
class BcmTestWedgeTomahawk3Platform : public BcmTestWedgePlatform {
 public:
  BcmTestWedgeTomahawk3Platform(
      std::vector<PortID> masterLogicalPortIds,
      int numPortsPerTranceiver)
      : BcmTestWedgePlatform(masterLogicalPortIds, numPortsPerTranceiver) {}
  ~BcmTestWedgeTomahawk3Platform() override {}

  bool isCosSupported() const override {
    return true;
  }

  bool v6MirrorTunnelSupported() const override {
    return true;
  }

  bool sflowSamplingSupported() const override {
    return true;
  }

  bool mirrorPktTruncationSupported() const override {
    return true;
  }

  bool canUseHostTableForHostRoutes() const override {
    return true;
  }
  std::list<FlexPortMode> getSupportedFlexPortModes() const override {
    // TODO(joseph5wu) Right now, we don't fully support flexport for TH3
    return {};
  }
  const PortQueue& getDefaultPortQueueSettings(
      cfg::StreamType streamType) const override;
  const PortQueue& getDefaultControlPlaneQueueSettings(
      cfg::StreamType streamType) const override;

  uint32_t getMMUBufferBytes() const override {
    // All TH3 platforms have 64MB MMU buffer
    return 64 * 1024 * 1024;
  }
  uint32_t getMMUCellBytes() const override {
    // All TH3 platforms have 254 byte cells
    return 254;
  }

  bool useQueueGportForCos() const override {
    return true;
  }

  uint32_t maxLabelStackDepth() const override {
    return 9;
  }

  bool isMultiPathLabelSwitchActionSupported() const override {
    return true;
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestWedgeTomahawk3Platform(BcmTestWedgeTomahawk3Platform const&) = delete;
  BcmTestWedgeTomahawk3Platform& operator=(
      BcmTestWedgeTomahawk3Platform const&) = delete;
};
} // namespace fboss
} // namespace facebook
