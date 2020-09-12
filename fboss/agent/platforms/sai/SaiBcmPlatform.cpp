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
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

std::string SaiBcmPlatform::getHwConfig() {
  auto& cfg =
      *config()->thrift.platform_ref()->chip_ref()->get_bcm().config_ref();
  std::vector<std::string> nameValStrs;
  for (const auto& entry : cfg) {
    nameValStrs.emplace_back(
        folly::to<std::string>(entry.first, '=', entry.second));
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
      allPortsinGroup.push_back(PortID(*port.mapping_ref()->id_ref()));
    }
  }
  return allPortsinGroup;
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

} // namespace facebook::fboss
