/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiM5120CSCPlatform.h"

#include "fboss/agent/hw/switch_asics/P200Asic.h"
#include "fboss/agent/platforms/common/m5120csc/M5120CSCPlatformMapping.h"

namespace facebook::fboss {

SaiM5120CSCPlatform::SaiM5120CSCPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiTajoPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<M5120CSCPlatformMapping>()
              : std::make_unique<M5120CSCPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiM5120CSCPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<P200Asic>(switchId, switchInfo);
  asic_->setDefaultStreamType(cfg::StreamType::UNICAST);
}

HwAsic* SaiM5120CSCPlatform::getAsic() const {
  return asic_.get();
}

std::string SaiM5120CSCPlatform::getHwConfig() {
  auto chipConfigType = config()->thrift.platform()->chip()->getType();
  if (chipConfigType != facebook::fboss::cfg::ChipConfig::Type::asicConfig) {
    return *config()->thrift.platform()->chip().value().get_asic().config();
  }

  auto& asicConfig =
      config()->thrift.platform()->chip().value().get_asicConfig();
  return asicConfig.common().value().get_jsonConfig();
}

std::vector<sai_system_port_config_t>
SaiM5120CSCPlatform::getInternalSystemPortConfig() const {
  CHECK(asic_) << " Asic must be set before getting sys port info";
  CHECK(asic_->getSwitchId()) << " Switch Id must be set before sys port info";
  auto switchIdVal = static_cast<uint32_t>(*asic_->getSwitchId());
  // Todo: copied from wedge400c, update later with correct port config from
  // vendor
  return {
      {11, switchIdVal, 0, 25, 100000, 8},
      {12, switchIdVal, 2, 25, 100000, 8},
      {13, switchIdVal, 4, 25, 100000, 8},
      {14, switchIdVal, 6, 25, 100000, 8},
      {15, switchIdVal, 8, 25, 100000, 8},
      {16, switchIdVal, 10, 25, 100000, 8},
      {6, switchIdVal, 0, 24, 1000, 8},
      {7, switchIdVal, 4, 24, 1000, 8},
      {8, switchIdVal, 6, 24, 1000, 8},
      {9, switchIdVal, 8, 24, 1000, 8},
      {10, switchIdVal, 1, 24, 1000, 8}};
}

SaiM5120CSCPlatform::~SaiM5120CSCPlatform() = default;

} // namespace facebook::fboss
