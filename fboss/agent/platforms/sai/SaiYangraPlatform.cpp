/*
 *  Copyright (c) 2023-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiYangraPlatform.h"
#include "fboss/agent/hw/switch_asics/ChenabAsic.h"
#include "fboss/agent/platforms/common/yangra/YangraPlatformMapping.h"

#include "fboss/agent/hw/sai/api/ArsApi.h"
#include "fboss/agent/hw/sai/api/ArsProfileApi.h"
#include "fboss/agent/hw/sai/api/MplsApi.h"
#include "fboss/agent/hw/sai/api/SystemPortApi.h"
#include "fboss/agent/hw/sai/api/TamApi.h"
#include "fboss/agent/hw/sai/api/VirtualRouterApi.h"

#include <algorithm>

namespace facebook::fboss {

// No Change
SaiYangraPlatform::SaiYangraPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<YangraPlatformMapping>()
              : std::make_unique<YangraPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiYangraPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    int16_t switchIndex,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac,
    std::optional<HwAsic::FabricNodeRole> /*role*/) {
  asic_ = std::make_unique<ChenabAsic>(
      switchType, switchId, switchIndex, systemPortRange, mac);
  asic_->setDefaultStreamType(cfg::StreamType::UNICAST);
}

HwAsic* SaiYangraPlatform::getAsic() const {
  return asic_.get();
}
const std::unordered_map<std::string, std::string>
SaiYangraPlatform::getSaiProfileVendorExtensionValues() const {
  return std::unordered_map<std::string, std::string>();
}

const std::set<sai_api_t>& SaiYangraPlatform::getSupportedApiList() const {
  static auto apis = getDefaultSwitchAsicSupportedApis();
  apis.erase(facebook::fboss::MplsApi::ApiType);
  apis.erase(facebook::fboss::VirtualRouterApi::ApiType);
  apis.erase(facebook::fboss::TamApi::ApiType);
  apis.erase(facebook::fboss::SystemPortApi::ApiType);
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  apis.erase(facebook::fboss::ArsApi::ApiType);
  apis.erase(facebook::fboss::ArsProfileApi::ApiType);
#endif
  return apis;
}

std::optional<SaiSwitchTraits::Attributes::AclFieldList>
SaiYangraPlatform::getAclFieldList() const {
  return std::nullopt;
}
std::string SaiYangraPlatform::getHwConfig() {
  std::string xml_filename = "/usr/share/sai_5600.xml";
  std::ifstream xml_file(xml_filename);
  std::string xml_config(
      (std::istreambuf_iterator<char>(xml_file)),
      std::istreambuf_iterator<char>());
  // std::cout << "Read config from: " << xml_filename << std::endl;
  // std::cout << "Content:" << std::endl << xml_config << std::endl;
  return xml_config;
}

bool SaiYangraPlatform::isSerdesApiSupported() const {
  return false;
}
std::vector<PortID> SaiYangraPlatform::getAllPortsInGroup(
    PortID /*portID*/) const {
  return {};
}
std::vector<FlexPortMode> SaiYangraPlatform::getSupportedFlexPortModes() const {
  return {
      FlexPortMode::ONEX400G,
      FlexPortMode::ONEX100G,
      FlexPortMode::ONEX40G,
      FlexPortMode::FOURX25G,
      FlexPortMode::FOURX10G,
      FlexPortMode::TWOX50G};
}
std::optional<sai_port_interface_type_t> SaiYangraPlatform::getInterfaceType(
    TransmitterTechnology /*transmitterTech*/,
    cfg::PortSpeed /*speed*/) const {
  return std::nullopt;
}

bool SaiYangraPlatform::supportInterfaceType() const {
  return false;
}

void SaiYangraPlatform::initLEDs() {}
SaiYangraPlatform::~SaiYangraPlatform() = default;

SaiSwitchTraits::CreateAttributes SaiYangraPlatform::getSwitchAttributes(
    bool mandatoryOnly,
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    BootType bootType) {
  auto attributes = SaiPlatform::getSwitchAttributes(
      mandatoryOnly, switchType, switchId, bootType);
  std::get<std::optional<SaiSwitchTraits::Attributes::HwInfo>>(attributes) =
      std::nullopt;
  return attributes;
}
} // namespace facebook::fboss
