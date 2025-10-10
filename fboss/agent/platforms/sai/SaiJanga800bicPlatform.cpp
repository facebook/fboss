/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiJanga800bicPlatform.h"

#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/platforms/common/janga800bic/Janga800bicPlatformMapping.h"

namespace {
/** Number of CPU ports configured for the Janga platform (one per core) */
constexpr auto kNumCpuPorts = 4;

/** CPU port speed in Mbps (10 Gbps) */
constexpr auto kCpuPortSpeed = 10000;

/** Number of Virtual Output Queues (VoQs) per CPU port */
constexpr auto kCpuPortNumVoqs = 8;
} // namespace

namespace facebook::fboss {

SaiJanga800bicPlatform::SaiJanga800bicPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Janga800bicPlatformMapping>()
              : std::make_unique<Janga800bicPlatformMapping>(
                    platformMappingStr),
          localMac) {}

void SaiJanga800bicPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Jericho3Asic>(switchId, switchInfo);
}

HwAsic* SaiJanga800bicPlatform::getAsic() const {
  return asic_.get();
}

/**
 * @brief Generates internal system port configuration for CPU ports
 *
 * Creates SAI system port configurations for all CPU ports on the Janga
 * platform. This method retrieves CPU port core and port index mappings from
 * the BCM configuration and generates corresponding SAI system port
 * configurations with the appropriate switch ID, core assignment, port speed,
 * and VoQ settings.

 *
 * @return std::vector<sai_system_port_config_t> Vector of SAI system port
 *         configurations for all CPU ports
 * @throws CHECK failure if ASIC or switch ID is not set, or if the number
 *         of configured CPU ports doesn't match kNumCpuPorts
 */
std::vector<sai_system_port_config_t>
SaiJanga800bicPlatform::getInternalSystemPortConfig() const {
  CHECK(asic_) << " Asic must be set before getting sys port info";
  CHECK(asic_->getSwitchId()) << " Switch Id must be set before sys port info";

  const uint32_t switchId = static_cast<uint32_t>(*asic_->getSwitchId());
  auto cpuPortsCoreAndPortIdx =
      getPlatformMapping()->getCpuPortsCoreAndPortIdx();

  CHECK(cpuPortsCoreAndPortIdx.size() == kNumCpuPorts)
      << "Create one CPU port for each core";

  std::vector<sai_system_port_config_t> sysPortConfig;
  for (auto [cpuPortID, coreAndPortIdx] : cpuPortsCoreAndPortIdx) {
    auto [core, port] = coreAndPortIdx;
    sysPortConfig.push_back(
        {cpuPortID, switchId, core, port, kCpuPortSpeed, kCpuPortNumVoqs});
  }
  return sysPortConfig;
}
SaiJanga800bicPlatform::~SaiJanga800bicPlatform() = default;

} // namespace facebook::fboss
