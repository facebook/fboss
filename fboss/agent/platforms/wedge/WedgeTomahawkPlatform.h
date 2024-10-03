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

#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/platforms/wedge/WedgePlatform.h"

#include <optional>

namespace facebook::fboss {

class PlatformProductInfo;

class WedgeTomahawkPlatform : public WedgePlatform {
 public:
  explicit WedgeTomahawkPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping,
      folly::MacAddress localMac);

  uint32_t getMMUBufferBytes() const override {
    // All WedgeTomahawk platforms have 16MB MMU buffer
    return 16 * 1024 * 1024;
  }
  uint32_t getMMUCellBytes() const override {
    // All WedgeTomahawk platforms have 208 byte cells
    return 208;
  }
  const PortQueue& getDefaultPortQueueSettings(
      cfg::StreamType streamType) const override;
  const PortQueue& getDefaultControlPlaneQueueSettings(
      cfg::StreamType streamType) const override;

  bool useQueueGportForCos() const override {
    return true;
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
      std::optional<HwAsic::FabricNodeRole> fabricNodeRole) override;
  std::unique_ptr<TomahawkAsic> asic_;
};

} // namespace facebook::fboss
