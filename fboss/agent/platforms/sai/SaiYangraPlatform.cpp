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

#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"
#include "fboss/agent/hw/sai/api/ArsApi.h"
#include "fboss/agent/hw/sai/api/MplsApi.h"
#include "fboss/agent/hw/sai/api/SystemPortApi.h"
#include "fboss/agent/hw/sai/api/TamApi.h"
#include "fboss/agent/hw/sai/api/VirtualRouterApi.h"

#include "fboss/agent/Utils.h"

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
  kv_map.insert(std::make_pair("SAI_KEY_PORT_AUTONEG_DEFAULT_OFF", "1"));
  kv_map.insert(std::make_pair("SAI_KEY_NOT_DROP_SMAC_DMAC_EQUAL", "1"));
#if !defined(CHENAB_SAI_SDK_VERSION_2505_34_0_1)
  kv_map.insert(std::make_pair("SAI_KEY_RECLAIM_BUFFER_ENABLED", "0"));
#else
  kv_map.insert(std::make_pair("SAI_KEY_RECLAIM_PG0_BUFFER_DISABLED", "1"));
#endif
  kv_map.insert(std::make_pair("SAI_KEY_TRAP_PACKETS_USING_CALLBACK", "1"));
  kv_map.insert(std::make_pair("SAI_KEY_ROUTE_METADATA_FIELD_SIZE", "5"));
  kv_map.insert(
      std::make_pair("SAI_KEY_CPU_PORT_PIPELINE_LOOKUP_L3_TRUST_MODE", "1"));
  kv_map.insert(std::make_pair("SAI_KEY_GET_OBJECT_KEY_EXCLUDE", "7"));
  kv_map.insert(std::make_pair("SAI_KEY_EXTERNAL_SXD_DRIVER_MANAGEMENT", "1"));
  kv_map.insert(std::make_pair("SAI_INTERNAL_LOOPBACK_TOGGLE_ENABLED", "1"));
  kv_map.insert(std::make_pair("SAI_KEY_ENABLE_HEALTH_DATA_TYPE_SER", "1"));
  utilCreateDir(getDirectoryUtil()->getCrashInfoDir());
  kv_map.insert(
      std::make_pair(
          "SAI_DUMP_STORE_PATH", getDirectoryUtil()->getCrashInfoDir()));
  kv_map.insert(std::make_pair("SAI_DUMP_STORE_AMOUNT", "1"));
  kv_map.insert(std::make_pair("SAI_KEY_HOSTIF_V2_ENABLED", "1"));
  kv_map.insert(std::make_pair("SAI_KEY_PRBS_ADMIN_TOGGLE_ENABLED", "1"));
  kv_map.insert(std::make_pair("SAI_KEY_ROUTE_METADATA_BIT_START", "0"));
  kv_map.insert(std::make_pair("SAI_KEY_PRBS_OVER_ISSU_ENABLED", "1"));
  kv_map.insert(std::make_pair("SAI_KEY_AR_ECMP_RANDOM_SPRAY_ENABLED", "1"));
  kv_map.insert(
      std::make_pair("SAI_KEY_ROUTE_METADATA_BIT_END", "4")); // 5 bit metadata
  // by default only non-discarded packet is counted in
  // SAI_PORT_STAT_IF_IN_UCAST_PKTS, set SAI_PORT_STAT_IF_IN_UCAST_PKTS to count
  // all received packet, including discarded packets
  kv_map.insert(std::make_pair("SAI_AGGREGATE_UCAST_DROPS", "1"));
  return kv_map;
}

const std::set<sai_api_t>& SaiYangraPlatform::getSupportedApiList() const {
  static auto apis = getDefaultSwitchAsicSupportedApis();
  apis.erase(facebook::fboss::MplsApi::ApiType);
  apis.erase(facebook::fboss::TamApi::ApiType);
  apis.erase(facebook::fboss::SystemPortApi::ApiType);
  return apis;
}

std::optional<SaiSwitchTraits::Attributes::AclFieldList>
SaiYangraPlatform::getAclFieldList() const {
  return std::nullopt;
}
std::string SaiYangraPlatform::getHwConfig() {
  std::string xml_filename =
      *config()->thrift.platform()->chip().value().get_asic().config();
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
  return true;
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
  // disable timeout based discards and retain only buffer discards
  std::get<std::optional<SaiSwitchTraits::Attributes::DisableSllAndHllTimeout>>(
      attributes) = true;

  return attributes;
}

HwSwitchWarmBootHelper* SaiYangraPlatform::getWarmBootHelper() {
  if (!wbHelper_) {
    wbHelper_ = std::make_unique<HwSwitchWarmBootHelper>(
        getAsic()->getSwitchIndex(),
        getDirectoryUtil()->getWarmBootDir(),
        "sai_adaptor_state_",
        false /* do not create warm boot data file */);
  }
  return wbHelper_.get();
}
} // namespace facebook::fboss
