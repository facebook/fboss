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

#include "fboss/agent/platforms/sai/SaiYangraPlatform.h"

namespace facebook::fboss {

class SaiYangra2Platform : public SaiYangraPlatform {
 public:
  SaiYangra2Platform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
  SaiYangra2Platform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      std::unique_ptr<PlatformMapping> platformMapping);
  ~SaiYangra2Platform() override;

  SaiSwitchTraits::CreateAttributes getSwitchAttributes(
      bool mandatoryOnly,
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      BootType bootType) override;

 private:
  void setupAsic(
      std::optional<int64_t> switchId,
      const cfg::SwitchInfo& switchInfo,
      std::optional<HwAsic::FabricNodeRole> role) override;
};

} // namespace facebook::fboss
