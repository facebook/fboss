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

#include "fboss/agent/hw/switch_asics/Tomahawk4Asic.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {

class BcmTestTomahawk4Platform : public BcmTestWedgePlatform {
 public:
  BcmTestTomahawk4Platform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping)
      : BcmTestWedgePlatform(
            std::move(productInfo),
            std::move(platformMapping)) {}
  ~BcmTestTomahawk4Platform() override {}

  bool canUseHostTableForHostRoutes() const override {
    return false;
  }
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {};
  }
  const PortQueue& getDefaultPortQueueSettings(
      cfg::StreamType streamType) const override;
  const PortQueue& getDefaultControlPlaneQueueSettings(
      cfg::StreamType streamType) const override;

  uint32_t getMMUBufferBytes() const override {
    return 2 * 234606 * 254;
  }
  uint32_t getMMUCellBytes() const override {
    return 254;
  }

  const PortPgConfig& getDefaultPortPgSettings() const override;
  const BufferPoolCfg& getDefaultPortIngressPoolSettings() const override;
  int getPortItm(BcmPort* bcmPort) const override;

  bool useQueueGportForCos() const override {
    return true;
  }

  HwAsic* getAsic() const override {
    return asic_.get();
  }

  bool usesYamlConfig() const override {
    return true;
  }

  bool supportsAddRemovePort() const override {
    return true;
  }

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      int16_t switchIndex,
      std::optional<cfg::Range64> systemPortRange,
      folly::MacAddress& mac,
      std::optional<HwAsic::FabricNodeRole> fabricNodeRole) override {
    CHECK(!fabricNodeRole.has_value());
    asic_ = std::make_unique<Tomahawk4Asic>(
        switchType, switchId, switchIndex, systemPortRange, mac);
  }
  // Forbidden copy constructor and assignment operator
  BcmTestTomahawk4Platform(BcmTestTomahawk4Platform const&) = delete;
  BcmTestTomahawk4Platform& operator=(BcmTestTomahawk4Platform const&) = delete;
  std::unique_ptr<Tomahawk4Asic> asic_;
};

} // namespace facebook::fboss
