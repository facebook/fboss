/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiMeru800biaPlatform.h"

#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/platforms/common/meru800bia/Meru800biaPlatformMapping.h"

namespace {
constexpr auto kCpuPortSpeed = 10000;
constexpr auto kSingleStageCpuPortNumVoqs = 8;
constexpr auto kDualStageCpuPortNumVoqs = 3;
} // namespace

namespace facebook::fboss {

SaiMeru800biaPlatform::SaiMeru800biaPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Meru800biaPlatformMapping>()
              : std::make_unique<Meru800biaPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiMeru800biaPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Jericho3Asic>(switchId, switchInfo);
}

HwAsic* SaiMeru800biaPlatform::getAsic() const {
  return asic_.get();
}

std::vector<sai_system_port_config_t>
SaiMeru800biaPlatform::getInternalSystemPortConfig() const {
  CHECK(asic_) << " Asic must be set before getting sys port info";
  CHECK(asic_->getSwitchId()) << " Switch Id must be set before sys port info";

  const uint32_t switchId = static_cast<uint32_t>(*asic_->getSwitchId());
  const uint32_t speed = kCpuPortSpeed;
  const uint32_t numVoqs = isDualStage3Q2QMode() ? kDualStageCpuPortNumVoqs
                                                 : kSingleStageCpuPortNumVoqs;
  auto cpuPortsCoreAndPortIdx =
      getPlatformMapping()->getCpuPortsCoreAndPortIdx();

  CHECK(
      cpuPortsCoreAndPortIdx.size() == 1 || cpuPortsCoreAndPortIdx.size() == 4)
      << "Create one CPU port for the ASIC or one CPU port for each core";

  std::vector<sai_system_port_config_t> sysPortConfig;
  for (auto [cpuPortID, coreAndPortIdx] : cpuPortsCoreAndPortIdx) {
    auto [core, port] = coreAndPortIdx;
    sysPortConfig.push_back({cpuPortID, switchId, core, port, speed, numVoqs});
  }
  return sysPortConfig;
}
SaiMeru800biaPlatform::~SaiMeru800biaPlatform() = default;

} // namespace facebook::fboss
