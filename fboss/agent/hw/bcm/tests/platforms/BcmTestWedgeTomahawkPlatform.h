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

#include "fboss/agent/hw/bcm/tests/platforms/BcmTestWedgePlatform.h"

namespace facebook { namespace fboss {
class BcmTestWedgeTomahawkPlatform : public BcmTestWedgePlatform {
public:
  BcmTestWedgeTomahawkPlatform(
    std::vector<int> masterLogicalPortIds,
    int numPortsPerTranceiver)
    : BcmTestWedgePlatform(masterLogicalPortIds, numPortsPerTranceiver) {}
  ~BcmTestWedgeTomahawkPlatform() override {}

  cfg::PortSpeed getMaxPortSpeed() override {
    return cfg::PortSpeed::HUNDREDG;
  }

  bool isCosSupported() const override {
    return true;
  }

  bool v6MirrorTunnelSupported() const override {
    return false;
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

private:
  // Forbidden copy constructor and assignment operator
  BcmTestWedgeTomahawkPlatform(BcmTestWedgeTomahawkPlatform const&) = delete;
  BcmTestWedgeTomahawkPlatform& operator=(
    BcmTestWedgeTomahawkPlatform const&) = delete;
};
}} // facebook::fboss
