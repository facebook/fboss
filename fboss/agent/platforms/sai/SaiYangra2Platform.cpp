/*
 *  Copyright (c) 2023-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiYangra2Platform.h"
#include "fboss/agent/hw/switch_asics/Chenab2Asic.h"
#include "fboss/agent/platforms/common/yangra2/Yangra2PlatformMapping.h"

namespace facebook::fboss {

SaiYangra2Platform::SaiYangra2Platform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiYangraPlatform(
          std::move(productInfo),
          localMac,
          platformMappingStr.empty()
              ? std::make_unique<Yangra2PlatformMapping>()
              : std::make_unique<Yangra2PlatformMapping>(platformMappingStr)) {}

SaiYangra2Platform::SaiYangra2Platform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    std::unique_ptr<PlatformMapping> platformMapping)
    : SaiYangraPlatform(
          std::move(productInfo),
          localMac,
          std::move(platformMapping)) {}

void SaiYangra2Platform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Chenab2Asic>(switchId, switchInfo);
  asic_->setDefaultStreamType(cfg::StreamType::UNICAST);
}

SaiSwitchTraits::CreateAttributes SaiYangra2Platform::getSwitchAttributes(
    bool mandatoryOnly,
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    BootType bootType) {
  auto attributes = SaiYangraPlatform::getSwitchAttributes(
      mandatoryOnly, switchType, switchId, bootType);
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
  std::get<std::optional<SaiSwitchTraits::Attributes::PtpMode>>(attributes) =
      SAI_PORT_PTP_MODE_SINGLE_STEP_TIMESTAMP;
#endif

  return attributes;
}

SaiYangra2Platform::~SaiYangra2Platform() = default;
} // namespace facebook::fboss
