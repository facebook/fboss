/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/common/utils/BcmYamlConfig.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

std::string SaiBcmPlatform::getHwConfig() {
  if (getAsic()->isSupported(HwAsic::Feature::HSDK)) {
    if (auto yamlConfig =
            config()->thrift.platform()->chip()->get_bcm().yamlConfig()) {
      if (supportsDynamicBcmConfig()) {
        BcmYamlConfig bcmYamlConfig;
        bcmYamlConfig.setBaseConfig(*yamlConfig);
        auto ports = config()->thrift.sw()->get_ports();
        bcmYamlConfig.modifyCoreMaps(
            getPlatformMapping()->getCorePinMapping(ports));
        return bcmYamlConfig.getConfig();
      }
      return *yamlConfig;
    }
    throw FbossError("Failed to get bcm yaml config from agent config");
  }
  auto& cfg = *config()->thrift.platform()->chip()->get_bcm().config();
  std::vector<std::string> nameValStrs;
  for (const auto& entry : cfg) {
    nameValStrs.emplace_back(
        folly::to<std::string>(entry.first, '=', entry.second));
    hwConfig_.emplace(std::make_pair(entry.first, entry.second));
  }
  auto hwConfig = folly::join('\n', nameValStrs);
  return hwConfig;
}

std::vector<PortID> SaiBcmPlatform::getAllPortsInGroup(PortID portID) const {
  std::vector<PortID> allPortsinGroup;
  if (const auto& platformPorts = getPlatformPorts(); !platformPorts.empty()) {
    const auto& portList =
        utility::getPlatformPortsByControllingPort(platformPorts, portID);
    for (const auto& port : portList) {
      allPortsinGroup.push_back(PortID(*port.mapping()->id()));
    }
  }
  return allPortsinGroup;
}

const char* SaiBcmPlatform::getHwConfigValue(const std::string& key) const {
  auto it = hwConfig_.find(key);
  return it == hwConfig_.end() ? nullptr : it->second.c_str();
}

std::optional<sai_port_interface_type_t> SaiBcmPlatform::getInterfaceType(
    TransmitterTechnology transmitterTech,
    cfg::PortSpeed speed) const {
  if (!getAsic()->isSupported(HwAsic::Feature::PORT_INTERFACE_TYPE)) {
    return std::nullopt;
  }
#if defined(IS_OSS)
  return std::nullopt;
#else
  static std::map<
      cfg::PortSpeed,
      std::map<TransmitterTechnology, sai_port_interface_type_t>>
      kSpeedAndMediaType2InterfaceType = {
          {cfg::PortSpeed::HUNDREDG,
           {{TransmitterTechnology::COPPER, SAI_PORT_INTERFACE_TYPE_CR4},
            {TransmitterTechnology::OPTICAL, SAI_PORT_INTERFACE_TYPE_CAUI},
            // What to default to
            {TransmitterTechnology::UNKNOWN, SAI_PORT_INTERFACE_TYPE_CAUI}}},
          {cfg::PortSpeed::FIFTYG,
           {{TransmitterTechnology::COPPER, SAI_PORT_INTERFACE_TYPE_CR2},
            {TransmitterTechnology::OPTICAL, SAI_PORT_INTERFACE_TYPE_CAUI},
            // What to default to
            {TransmitterTechnology::UNKNOWN, SAI_PORT_INTERFACE_TYPE_CR2}}},
          {cfg::PortSpeed::FORTYG,
           {{TransmitterTechnology::COPPER, SAI_PORT_INTERFACE_TYPE_CR4},
            {TransmitterTechnology::OPTICAL, SAI_PORT_INTERFACE_TYPE_XLAUI},
            // What to default to
            {TransmitterTechnology::UNKNOWN, SAI_PORT_INTERFACE_TYPE_XLAUI}}},
          {cfg::PortSpeed::TWENTYFIVEG,
           {{TransmitterTechnology::COPPER, SAI_PORT_INTERFACE_TYPE_CR},
            {TransmitterTechnology::OPTICAL, SAI_PORT_INTERFACE_TYPE_CAUI},
            // What to default to
            {TransmitterTechnology::UNKNOWN, SAI_PORT_INTERFACE_TYPE_CR}}},
          {cfg::PortSpeed::TWENTYG,
           {{TransmitterTechnology::COPPER, SAI_PORT_INTERFACE_TYPE_CR},
            // We don't expect 20G optics
            // What to default to
            {TransmitterTechnology::UNKNOWN, SAI_PORT_INTERFACE_TYPE_CR}}},
          {cfg::PortSpeed::XG,
           {{TransmitterTechnology::COPPER, SAI_PORT_INTERFACE_TYPE_CR},
            {TransmitterTechnology::OPTICAL, SAI_PORT_INTERFACE_TYPE_SFI},
            // What to default to
            {TransmitterTechnology::UNKNOWN, SAI_PORT_INTERFACE_TYPE_CR}}},
          {cfg::PortSpeed::GIGE,
           {{TransmitterTechnology::COPPER, SAI_PORT_INTERFACE_TYPE_GMII},
            // We don't expect 1G optics
            // What to default to
            {TransmitterTechnology::UNKNOWN, SAI_PORT_INTERFACE_TYPE_GMII}}}};
  auto mediaType2InterfaceTypeIter =
      kSpeedAndMediaType2InterfaceType.find(speed);
  if (mediaType2InterfaceTypeIter == kSpeedAndMediaType2InterfaceType.end()) {
    throw FbossError(
        "unsupported speed for interface type retrieval : ", speed);
  }
  auto interfaceTypeIter =
      mediaType2InterfaceTypeIter->second.find(transmitterTech);
  if (interfaceTypeIter == mediaType2InterfaceTypeIter->second.end()) {
    throw FbossError(
        "unsupported media type for interface type retrieval : ",
        transmitterTech);
  }
  return interfaceTypeIter->second;
#endif
}

bool SaiBcmPlatform::needPortVcoChange(
    cfg::PortSpeed oldSpeed,
    phy::FecMode oldFec,
    cfg::PortSpeed newSpeed,
    phy::FecMode newFec) const {
  return getPortVcoFrequency(oldSpeed, oldFec) !=
      getPortVcoFrequency(newSpeed, newFec);
}

phy::VCOFrequency SaiBcmPlatform::getPortVcoFrequency(
    cfg::PortSpeed speed,
    phy::FecMode fec) const {
  switch (speed) {
    case cfg::PortSpeed::FOURHUNDREDG:
      FOLLY_FALLTHROUGH;
    case cfg::PortSpeed::TWOHUNDREDG:
      return phy::VCOFrequency::VCO_26_5625GHZ;
    case cfg::PortSpeed::HUNDREDG:
      switch (fec) {
        case phy::FecMode::RS544:
          FOLLY_FALLTHROUGH;
        case phy::FecMode::RS544_2N:
          return phy::VCOFrequency::VCO_26_5625GHZ;
        case phy::FecMode::NONE:
          FOLLY_FALLTHROUGH;
        case phy::FecMode::CL74:
          FOLLY_FALLTHROUGH;
        case phy::FecMode::CL91:
          FOLLY_FALLTHROUGH;
        case phy::FecMode::RS545:
          FOLLY_FALLTHROUGH;
        case phy::FecMode::RS528:
          return phy::VCOFrequency::VCO_25_78125GHZ;
      }
    case cfg::PortSpeed::FIFTYG:
      FOLLY_FALLTHROUGH;
    case cfg::PortSpeed::FIFTYTHREEPOINTONETWOFIVEG:
      FOLLY_FALLTHROUGH;
    case cfg::PortSpeed::TWENTYFIVEG:
      switch (fec) {
        case phy::FecMode::RS544:
          FOLLY_FALLTHROUGH;
        case phy::FecMode::RS544_2N:
          return phy::VCOFrequency::VCO_26_5625GHZ;
        case phy::FecMode::NONE:
          FOLLY_FALLTHROUGH;
        case phy::FecMode::CL74:
          FOLLY_FALLTHROUGH;
        case phy::FecMode::CL91:
          FOLLY_FALLTHROUGH;
        case phy::FecMode::RS545:
          FOLLY_FALLTHROUGH;
        case phy::FecMode::RS528:
          return phy::VCOFrequency::VCO_25_78125GHZ;
      }
    case cfg::PortSpeed::FORTYG:
      FOLLY_FALLTHROUGH;
    case cfg::PortSpeed::TWENTYG:
      FOLLY_FALLTHROUGH;
    case cfg::PortSpeed::XG:
      return phy::VCOFrequency::VCO_20_625GHZ;
    case cfg::PortSpeed::GIGE:
      FOLLY_FALLTHROUGH;
    case cfg::PortSpeed::EIGHTHUNDREDG:
    case cfg::PortSpeed::DEFAULT:
      return phy::VCOFrequency::UNKNOWN;
  }
  return phy::VCOFrequency::UNKNOWN;
}

} // namespace facebook::fboss
