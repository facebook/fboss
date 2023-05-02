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

#include "fboss/agent/platforms/tests/utils/BcmTestWedgePlatform.h"

namespace facebook::fboss {

class Trident2Asic;
class PlatformProductInfo;

class BcmTestWedge40Platform : public BcmTestWedgePlatform {
 public:
  explicit BcmTestWedge40Platform(std::unique_ptr<PlatformProductInfo> product);
  ~BcmTestWedge40Platform() override;

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

  HwAsic* getAsic() const override;

  void initLEDs(int unit) override;

  bool verifyLEDStatus(PortID port, bool up) override;

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      std::optional<cfg::Range64> systemPortRange,
      folly::MacAddress& mac) override;
  // Forbidden copy constructor and assignment operator
  BcmTestWedge40Platform(BcmTestWedge40Platform const&) = delete;
  BcmTestWedge40Platform& operator=(BcmTestWedge40Platform const&) = delete;

  std::unique_ptr<BcmTestPort> createTestPort(PortID id) override;
  std::unique_ptr<Trident2Asic> asic_;
};

} // namespace facebook::fboss
