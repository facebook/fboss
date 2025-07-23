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

#include <re2/re2.h>

namespace {
/** Number of CPU ports configured for the Janga platform (one per core) */
constexpr auto kNumCpuPorts = 4;

/** CPU ucode port numbers used by the Janga platform (chip config P1877668165)
 * These ports are mapped to cores 0-3 respectively */
constexpr std::array<uint32_t, kNumCpuPorts> kCpuUcodePorts = {
    0, /**< CPU port 0 - Core 0 */
    200, /**< CPU port 1 - Core 1 */
    201, /**< CPU port 2 - Core 2 */
    202 /**< CPU port 3 - Core 3 */
};

/** CPU port speed in Mbps (10 Gbps) */
constexpr auto kCpuPortSpeed = 10000;

/** Number of Virtual Output Queues (VoQs) per CPU port */
constexpr auto kCpuPortNumVoqs = 8;

/** Regex pattern for parsing CPU port configuration from BCM config
 * Matches format: "CPU.<channel>:core_<core>.<port>"
 * Captures: channel number, core index (0-3), port index within core */
constexpr auto kCpuPortConfig =
    "CPU.([0-9]{1,2})\\:core_([0-3])\\.([0-9]{1,2})";

/** Compiled regex for CPU port configuration parsing */
static const re2::RE2 kCpuPortConfigRegex(kCpuPortConfig);
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
 * @brief Retrieves CPU port core and port index mapping from BCM configuration
 *
 * Parses the BCM configuration to extract CPU port assignments to specific
 * cores and port indices. This method looks for ucode_port configurations
 * matching the kCpuUcodePorts array and extracts the core and port index
 * information using regex pattern matching.
 *
 * @return std::map<uint32_t, std::pair<uint32_t, uint32_t>> Map where:
 *         - Key: CPU port ID (0-3)
 *         - Value: Pair of (core index, port index within core)
 */
std::map<uint32_t, std::pair<uint32_t, uint32_t>>
SaiJanga800bicPlatform::getCpuPortsCoreAndPortIdx() const {
  std::map<uint32_t, std::pair<uint32_t, uint32_t>> cpuPortsCoreAndPortIdx;
  const auto& bcmConfig = getConfig()
                              ->thrift.platform()
                              ->chip()
                              ->get_asicConfig()
                              .common()
                              ->get_config();
  for (const auto& [key, value] : std::as_const(bcmConfig)) {
    for (auto cpuPortID = 0; cpuPortID < kCpuUcodePorts.size(); cpuPortID++) {
      auto cpuUcodePort = kCpuUcodePorts[cpuPortID];
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
  auto cpuPortsCoreAndPortIdx = getCpuPortsCoreAndPortIdx();

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
