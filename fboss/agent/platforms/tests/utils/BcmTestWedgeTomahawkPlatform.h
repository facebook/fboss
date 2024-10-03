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

#include "fboss/lib/platforms/PlatformProductInfo.h"

#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"

namespace facebook::fboss {

class BcmTestWedgeTomahawkPlatform : public BcmTestWedgePlatform {
 public:
  BcmTestWedgeTomahawkPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping)
      : BcmTestWedgePlatform(
            std::move(productInfo),
            std::move(platformMapping)) {}
  ~BcmTestWedgeTomahawkPlatform() override {}

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
    return true;
  }

  bool canUseHostTableForHostRoutes() const override {
    /*
     * We run some TH nodes with host route optimization enabled (wedge100,
     * Galaxy) and others with this optimization disabled (FAv3). Ideally we
     * would run tests with both variations. Since that is expensive, we just
     * run tests with a more permissive (in terms of route scale) variation. If
     * we care to run with both variations, we can make this a command line flag
     * like we do in prod.
     */
    return false;
  }

  HwAsic* getAsic() const override {
    return asic_.get();
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
    asic_ = std::make_unique<TomahawkAsic>(
        switchType, switchId, switchIndex, systemPortRange, mac);
  }
  // Forbidden copy constructor and assignment operator
  BcmTestWedgeTomahawkPlatform(BcmTestWedgeTomahawkPlatform const&) = delete;
  BcmTestWedgeTomahawkPlatform& operator=(BcmTestWedgeTomahawkPlatform const&) =
      delete;
  std::unique_ptr<TomahawkAsic> asic_;
};

} // namespace facebook::fboss
