/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/GenericSaiTajoPlatform.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

GenericSaiTajoPlatform::GenericSaiTajoPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<PlatformMapping> platformMapping,
    folly::MacAddress localMac)
    : SaiTajoPlatform(
          std::move(productInfo),
          std::move(platformMapping),
          localMac) {}

GenericSaiTajoPlatform::~GenericSaiTajoPlatform() = default;

void GenericSaiTajoPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  std::optional<cfg::SdkVersion> sdkVersion;
  auto agentConfig = config();
  if (agentConfig->thrift.sw()->sdkVersion().has_value()) {
    sdkVersion = agentConfig->thrift.sw()->sdkVersion().value();
  }
  asic_ = HwAsic::makeAsic(
      switchId.value_or(0), switchInfo, sdkVersion, fabricNodeRole);
}

HwAsic* GenericSaiTajoPlatform::getAsic() const {
  return asic_.get();
}

} // namespace facebook::fboss
