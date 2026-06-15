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

#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"

namespace facebook::fboss {

class GenericSaiBcmPlatform : public SaiBcmPlatform {
 public:
  GenericSaiBcmPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping,
      folly::MacAddress localMac);
  ~GenericSaiBcmPlatform() override;

  HwAsic* getAsic() const override;
  // TODO: Move these methods to ASIC info instead of platform info.
  uint32_t numLanesPerCore() const override;
  uint32_t numCellsAvailable() const override;
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override;
  bool supportInterfaceType() const override;
  void initLEDs() override {}

 private:
  void setupAsic(
      std::optional<int64_t> switchId,
      const cfg::SwitchInfo& switchInfo,
      std::optional<HwAsic::FabricNodeRole> fabricNodeRole) override;

  std::unique_ptr<HwAsic> asic_;
};

} // namespace facebook::fboss
