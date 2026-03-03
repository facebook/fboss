/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmJ4SimPlatform.h"

#include "fboss/agent/hw/switch_asics/Jericho4Asic.h"
#include "fboss/agent/platforms/common/j4sim/J4SimPlatformMapping.h"

namespace {
constexpr auto kCpuPortSpeed = 10000;
constexpr auto kSingleStageCpuPortNumVoqs = 8;
constexpr auto kDualStageCpuPortNumVoqs = 3;
} // namespace

namespace facebook::fboss {

SaiBcmJ4SimPlatform::SaiBcmJ4SimPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<J4SimPlatformMapping>()
              : std::make_unique<J4SimPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiBcmJ4SimPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Jericho4Asic>(switchId, switchInfo);
}

HwAsic* SaiBcmJ4SimPlatform::getAsic() const {
  return asic_.get();
}

std::vector<sai_system_port_config_t>
SaiBcmJ4SimPlatform::getInternalSystemPortConfig() const {
  CHECK(asic_) << " Asic must be set before getting sys port info";
  CHECK(asic_->getSwitchId()) << " Switch Id must be set before sys port info";

  const uint32_t switchId = static_cast<uint32_t>(*asic_->getSwitchId());
  const uint32_t speed = kCpuPortSpeed;
  const uint32_t numVoqs = isDualStage3Q2QMode() ? kDualStageCpuPortNumVoqs
                                                 : kSingleStageCpuPortNumVoqs;
  auto cpuPortsCoreAndPortIdx =
      getPlatformMapping()->getCpuPortsCoreAndPortIdx();

  CHECK(
      cpuPortsCoreAndPortIdx.size() == 1 ||
      cpuPortsCoreAndPortIdx.size() == 4 || cpuPortsCoreAndPortIdx.size() == 8)
      << "Create one CPU port for the ASIC or one CPU port for each core";

  std::vector<sai_system_port_config_t> sysPortConfig;
  for (auto [cpuPortID, coreAndPortIdx] : cpuPortsCoreAndPortIdx) {
    auto [core, port] = coreAndPortIdx;
    sysPortConfig.push_back({cpuPortID, switchId, core, port, speed, numVoqs});
  }
  return sysPortConfig;
}
SaiBcmJ4SimPlatform::~SaiBcmJ4SimPlatform() = default;

} // namespace facebook::fboss
