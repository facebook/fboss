/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiWedge800CACTPlatform.h"

#include "fboss/agent/hw/switch_asics/YubaAsic.h"
#include "fboss/agent/platforms/common/wedge800cact/Wedge800CACTPlatformMapping.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace facebook::fboss {

SaiWedge800CACTPlatform::SaiWedge800CACTPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiTajoPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Wedge800CACTPlatformMapping>()
              : std::make_unique<Wedge800CACTPlatformMapping>(
                    platformMappingStr),
          localMac) {}

void SaiWedge800CACTPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<YubaAsic>(switchId, switchInfo);
  asic_->setDefaultStreamType(cfg::StreamType::UNICAST);
}

HwAsic* SaiWedge800CACTPlatform::getAsic() const {
  return asic_.get();
}

std::string SaiWedge800CACTPlatform::getHwConfig() {
  auto chipConfigType = config()->thrift.platform()->chip()->getType();
  if (chipConfigType != facebook::fboss::cfg::ChipConfig::Type::asicConfig) {
    // TODO(vsp) : Remove this if check once we move ASIC config in
    // G200 simulator agent_unified.conf to v2 format.
    return *config()->thrift.platform()->chip().value().get_asic().config();
  }

  auto& asicConfig =
      config()->thrift.platform()->chip().value().get_asicConfig();
  return asicConfig.common().value().get_jsonConfig();
}

std::vector<PortID> SaiWedge800CACTPlatform::getAllPortsInGroup(
    PortID portID) const {
  std::vector<PortID> allPortsinGroup;
  if (const auto& platformPorts = getPlatformPorts(); !platformPorts.empty()) {
    const auto& portList =
        utility::getPlatformPortsByControllingPort(platformPorts, portID);
    for (const auto& port : portList) {
      allPortsinGroup.emplace_back(*port.mapping()->id());
    }
  }
  return allPortsinGroup;
}

SaiWedge800CACTPlatform::~SaiWedge800CACTPlatform() = default;

} // namespace facebook::fboss
