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

#include <re2/re2.h>

namespace {
constexpr std::array<int, 4> kSingleStageCpuUcodePorts = {0, 44, 45, 46};
constexpr std::array<int, 4> kDualStageCpuUcodePorts = {0, 45, 46, 47};
constexpr auto kCpuPortSpeed = 10000;
constexpr auto kSingleStageCpuPortNumVoqs = 8;
constexpr auto kDualStageCpuPortNumVoqs = 3;
constexpr auto kCpuPortConfig =
    "CPU.([0-9]{1,2})\\:core_([0-3])\\.([0-9]{1,2})";
static const re2::RE2 kCpuPortConfigRegex(kCpuPortConfig);
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

std::map<uint32_t, std::pair<uint32_t, uint32_t>>
SaiMeru800biaPlatform::getCpuPortsCoreAndPortIdx() const {
  std::map<uint32_t, std::pair<uint32_t, uint32_t>> cpuPortsCoreAndPortIdx;
  const auto& bcmConfig = getConfig()
                              ->thrift.platform()
                              ->chip()
                              ->get_asicConfig()
                              .common()
                              ->get_config();
  const auto& cpuUcodePorts = isDualStage3Q2QMode() ? kDualStageCpuUcodePorts
                                                    : kSingleStageCpuUcodePorts;
  for (const auto& [key, value] : std::as_const(bcmConfig)) {
    for (auto cpuPortID = 0; cpuPortID < cpuUcodePorts.size(); cpuPortID++) {
      auto cpuUcodePort = cpuUcodePorts[cpuPortID];
      if (key ==
          folly::to<std::string>("ucode_port_", cpuUcodePort, ".BCM8889X")) {
        uint32_t channel = 0, coreIdx = 0, corePortIdx = 0;
        if (re2::RE2::PartialMatch(
                value, kCpuPortConfigRegex, &channel, &coreIdx, &corePortIdx)) {
        } else {
          XLOG(FATAL) << "Failed to match CPU port config: " << value;
        }
        cpuPortsCoreAndPortIdx[cpuPortID] =
            std::make_pair(coreIdx, corePortIdx);
      }
    }
  }
  return cpuPortsCoreAndPortIdx;
}

std::vector<sai_system_port_config_t>
SaiMeru800biaPlatform::getInternalSystemPortConfig() const {
  CHECK(asic_) << " Asic must be set before getting sys port info";
  CHECK(asic_->getSwitchId()) << " Switch Id must be set before sys port info";

  const uint32_t switchId = static_cast<uint32_t>(*asic_->getSwitchId());
  const uint32_t speed = kCpuPortSpeed;
  const uint32_t numVoqs = isDualStage3Q2QMode() ? kDualStageCpuPortNumVoqs
                                                 : kSingleStageCpuPortNumVoqs;
  auto cpuPortsCoreAndPortIdx = getCpuPortsCoreAndPortIdx();

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
