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

SaiYangraPlatform::SaiYangraPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    std::unique_ptr<PlatformMapping> platformMapping)
    : SaiPlatform(
          std::move(productInfo),
          std::move(platformMapping),
          localMac) {}

void SaiYangraPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<ChenabAsic>(switchId, switchInfo);
  asic_->setDefaultStreamType(cfg::StreamType::UNICAST);
}

HwAsic* SaiYangraPlatform::getAsic() const {
  return asic_.get();
}
const std::unordered_map<std::string, std::string>
SaiYangraPlatform::getSaiProfileVendorExtensionValues() const {
  std::unordered_map<std::string, std::string> kv_map;
  kv_map.insert(std::make_pair("SAI_KEY_AUTO_POPULATE_PORT_DB", "1"));
  kv_map.insert(std::make_pair("SAI_KEY_NOT_DROP_SMAC_DMAC_EQUAL", "1"));
  kv_map.insert(std::make_pair("SAI_KEY_RECLAIM_BUFFER_ENABLED", "0"));
  kv_map.insert(std::make_pair("SAI_KEY_TRAP_PACKETS_USING_CALLBACK", "1"));
  return kv_map;
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
  std::string xml_filename =
      *config()->thrift.platform()->get_chip().get_asic().config();
  std::string base_filename =
      xml_filename.substr(0, xml_filename.find(".xml") + 4);
  std::ifstream xml_file(base_filename);
  std::string xml_config(
      (std::istreambuf_iterator<char>(xml_file)),
      std::istreambuf_iterator<char>());
  // std::cout << "Read config from: " << xml_filename << std::endl;
  // std::cout << "Content:" << std::endl << xml_config << std::endl;
  if (xml_filename.find(";disable_lb_filter") != std::string::npos) {
    std::string keyTag = "</issu-enabled>";
    std::string newKeyTag = "<lb_filter_disable>1</lb_filter_disable>";
    int indentSpaces = 8;

    size_t pos = xml_config.find(keyTag);
    if (pos != std::string::npos) {
      xml_config.insert(
          pos + keyTag.length(), std::string(indentSpaces, ' ') + newKeyTag);
    } else {
      std::cerr << "Tag " << keyTag << " not found in XML." << std::endl;
    }
  }
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
