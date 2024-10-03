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

#include "fboss/agent/hw/switch_asics/Tomahawk3Asic.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {

class BcmTestWedgeTomahawk3Platform : public BcmTestWedgePlatform {
 public:
  BcmTestWedgeTomahawk3Platform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping)
      : BcmTestWedgePlatform(
            std::move(productInfo),
            std::move(platformMapping)) {}
  ~BcmTestWedgeTomahawk3Platform() override {}

  bool canUseHostTableForHostRoutes() const override {
    return false;
  }
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    // TODO(joseph5wu) Right now, we don't fully support flexport for TH3
    return {};
  }
  const PortQueue& getDefaultPortQueueSettings(
      cfg::StreamType streamType) const override;
  const PortQueue& getDefaultControlPlaneQueueSettings(
      cfg::StreamType streamType) const override;
  const PortPgConfig& getDefaultPortPgSettings() const override;
  const BufferPoolCfg& getDefaultPortIngressPoolSettings() const override;

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

  HwAsic* getAsic() const override {
    return asic_.get();
  }

  bool supportsAddRemovePort() const override {
    return true;
  }
  int getPortItm(BcmPort* bcmPort) const override;

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      int16_t switchIndex,
      std::optional<cfg::Range64> systemPortRange,
      folly::MacAddress& mac,
      std::optional<HwAsic::FabricNodeRole> fabricNodeRole) override {
    CHECK(!fabricNodeRole.has_value());
    asic_ = std::make_unique<Tomahawk3Asic>(
        switchType, switchId, switchIndex, systemPortRange, mac);
  }
  // Forbidden copy constructor and assignment operator
  BcmTestWedgeTomahawk3Platform(BcmTestWedgeTomahawk3Platform const&) = delete;
  BcmTestWedgeTomahawk3Platform& operator=(
      BcmTestWedgeTomahawk3Platform const&) = delete;
  std::unique_ptr<Tomahawk3Asic> asic_;
};

} // namespace facebook::fboss
