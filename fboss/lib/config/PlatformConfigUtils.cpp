/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/config/PlatformConfigUtils.h"

#include "fboss/agent/FbossError.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

namespace {
using namespace facebook::fboss::phy;

void recurseGetPinsByChipType(
    const PinConnection& pinConn,
    const std::map<std::string, DataPlanePhyChip>& chipsMap,
    std::vector<Pin>& pins) {
  if (chipsMap.find(*pinConn.a_ref()->chip_ref()) != chipsMap.end()) {
    Pin temp;
    temp.set_end(*pinConn.a_ref());
    pins.push_back(temp);
    return;
  }

  auto zPinRef = pinConn.z_ref();
  if (!zPinRef) {
    return;
  }
  auto zPin = *zPinRef;
  if (zPin.getType() == Pin::Type::end) {
    if (chipsMap.find(*zPin.get_end().chip_ref()) != chipsMap.end()) {
      Pin temp;
      temp.set_end(zPin.get_end());
      pins.push_back(temp);
      return;
    }
  } else if (zPin.getType() == Pin::Type::junction) {
    auto zJunction = zPin.get_junction();
    if (chipsMap.find(*zJunction.system_ref()->chip_ref()) != chipsMap.end()) {
      Pin temp;
      temp.set_junction(zJunction);
      pins.push_back(temp);
      return;
    }
    for (const auto& connection : *zPin.get_junction().line_ref()) {
      recurseGetPinsByChipType(connection, chipsMap, pins);
    }
  }
}

std::vector<Pin> getPinsByChipType(
    const std::map<std::string, DataPlanePhyChip>& chipsMap,
    const std::vector<PinConnection>& pinConns,
    DataPlanePhyChipType type) {
  // First get all the expected chip type into a map to make search easier
  std::map<std::string, DataPlanePhyChip> filteredChips;
  for (const auto itChip : chipsMap) {
    if (*itChip.second.type_ref() == type) {
      filteredChips[itChip.first] = itChip.second;
    }
  }

  // use the recurse function to get all the expected chip pin
  std::vector<Pin> results;
  for (const auto& pinConn : pinConns) {
    recurseGetPinsByChipType(pinConn, filteredChips, results);
  }
  return results;
}

void checkPinType(const Pin& pin, const DataPlanePhyChipType& expectedType) {
  switch (expectedType) {
    case DataPlanePhyChipType::IPHY:
      if (pin.getType() != Pin::Type::end) {
        throw facebook::fboss::FbossError("Unsupported pin type for iphy");
      }
      break;
    case DataPlanePhyChipType::XPHY:
      if (pin.getType() != Pin::Type::junction) {
        throw facebook::fboss::FbossError("Unsupported pin type for xphy");
      }
      break;
    case DataPlanePhyChipType::TRANSCEIVER:
      if (pin.getType() != Pin::Type::end) {
        throw facebook::fboss::FbossError(
            "Unsupported pin type for transceiver");
      }
      break;
    default:
      throw facebook::fboss::FbossError(
          "Unsupported pin type:",
          apache::thrift::util::enumNameSafe(expectedType));
  }
}

std::string getChipName(const Pin& pin) {
  if (pin.getType() == Pin::Type::junction) {
    return *pin.get_junction().system_ref()->chip_ref();
  }
  return *pin.get_end().chip_ref();
}
} // unnamed namespace

namespace facebook::fboss::utility {

std::vector<phy::PinID> getTransceiverLanes(
    const cfg::PlatformPortEntry& port,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap,
    std::optional<cfg::PortProfileID> profileID) {
  std::vector<phy::PinID> lanes;
  if (profileID && profileID != cfg::PortProfileID::PROFILE_DEFAULT) {
    auto itPortCfg = port.supportedProfiles_ref()->find(*profileID);
    if (itPortCfg == port.supportedProfiles_ref()->end()) {
      throw FbossError(
          "Port: ",
          *port.mapping_ref()->name_ref(),
          " doesn't support profile:",
          apache::thrift::util::enumNameSafe(*profileID));
    }
    if (auto tcvr = itPortCfg->second.pins_ref()->transceiver_ref()) {
      for (const auto& pinCfg : *tcvr) {
        lanes.push_back(*pinCfg.id_ref());
      }
    }
  } else {
    // If it's not looking for the lanes list based on profile, return the
    // static lane mapping
    auto pins = getPinsByChipType(
        chipsMap,
        *port.mapping_ref()->pins_ref(),
        phy::DataPlanePhyChipType::TRANSCEIVER);
    // the return pins should always be PinID for transceiver
    for (auto pin : pins) {
      checkPinType(pin, phy::DataPlanePhyChipType::TRANSCEIVER);
      lanes.push_back(pin.get_end());
    }
  }
  return lanes;
}

std::map<int32_t, phy::PolaritySwap> getIphyPolaritySwapMap(
    const std::vector<phy::PinConnection>& pinConnections) {
  std::map<int32_t, phy::PolaritySwap> polaritySwapMap;

  for (const auto& pinConnection : pinConnections) {
    if (pinConnection.polaritySwap_ref()) {
      polaritySwapMap.emplace(
          *pinConnection.a_ref()->lane_ref(),
          *pinConnection.polaritySwap_ref());
    }
  }

  return polaritySwapMap;
}

std::map<int32_t, phy::LaneConfig> getIphyLaneConfigs(
    const std::vector<phy::PinConfig>& iphyPinConfigs) {
  std::map<int32_t, phy::LaneConfig> laneConfigs;
  for (auto pinConfig : iphyPinConfigs) {
    if (!pinConfig.tx_ref()) {
      continue;
    }
    phy::LaneConfig laneConfig;
    laneConfig.tx = *pinConfig.tx_ref();
    laneConfigs.emplace(*pinConfig.id_ref()->lane_ref(), laneConfig);
  }

  return laneConfigs;
}

std::map<int32_t, phy::PolaritySwap> getXphyLinePolaritySwapMap(
    const std::vector<phy::PinConnection>& pinConnections,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap) {
  std::map<int32_t, phy::PolaritySwap> xphyPolaritySwapMap;
  const auto& xphyPinList = getPinsByChipType(
      chipsMap, pinConnections, phy::DataPlanePhyChipType::XPHY);
  for (const auto& pin : xphyPinList) {
    checkPinType(pin, phy::DataPlanePhyChipType::XPHY);
    for (const auto& connection : *pin.get_junction().line_ref()) {
      if (auto pn = connection.polaritySwap_ref()) {
        xphyPolaritySwapMap.emplace(*connection.a_ref()->lane_ref(), *pn);
      }
    }
  }
  return xphyPolaritySwapMap;
}

std::vector<phy::PinID> getOrderedIphyLanes(
    const cfg::PlatformPortEntry& port,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap,
    std::optional<cfg::PortProfileID> profileID) {
  std::vector<phy::PinID> lanes;
  if (profileID && profileID != cfg::PortProfileID::PROFILE_DEFAULT) {
    auto itrPortCfg = port.supportedProfiles_ref()->find(*profileID);
    if (itrPortCfg == port.supportedProfiles_ref()->end()) {
      throw FbossError(
          "Port: ",
          *port.mapping_ref()->name_ref(),
          " doesn't support profile:",
          apache::thrift::util::enumNameSafe(*profileID));
    }
    for (const auto& pinCfg : *itrPortCfg->second.pins_ref()->iphy_ref()) {
      lanes.push_back(*pinCfg.id_ref());
    }
  } else {
    // If it's not looking for the lanes list based on profile, return the
    // static lane mapping
    auto pins = getPinsByChipType(
        chipsMap,
        *port.mapping_ref()->pins_ref(),
        phy::DataPlanePhyChipType::IPHY);
    // the return pins should always be PinID for transceiver
    for (auto pin : pins) {
      checkPinType(pin, phy::DataPlanePhyChipType::IPHY);
      lanes.push_back(pin.get_end());
    }
  }
  if (lanes.empty()) {
    throw FbossError(
        "Port: ", *port.mapping_ref()->name_ref(), " doesn't have iphy lanes");
  }
  // Sort by lane
  std::sort(lanes.begin(), lanes.end(), [](const auto& lPin, const auto& rPin) {
    return *lPin.lane_ref() < *rPin.lane_ref();
  });
  return lanes;
}

// Get subsidiary PortID list based on controlling port
std::map<PortID, std::vector<PortID>> getSubsidiaryPortIDs(
    const std::map<int32_t, cfg::PlatformPortEntry>& platformPorts) {
  std::map<PortID, std::vector<PortID>> results;
  for (auto itPort : platformPorts) {
    auto controlPortID = facebook::fboss::PortID(
        *itPort.second.mapping_ref()->controllingPort_ref());
    if (results.find(controlPortID) == results.end()) {
      results[controlPortID] = std::vector<PortID>();
    }
    // Note that the subsidiary_ports map includes the controlling port
    results[controlPortID].push_back(facebook::fboss::PortID(itPort.first));
  }

  // Sort subsidiary port ids
  for (auto itControlPort : results) {
    std::sort(
        itControlPort.second.begin(),
        itControlPort.second.end(),
        [](const auto& lPortID, const auto& rPortID) {
          return lPortID < rPortID;
        });
  }
  return results;
}

std::vector<cfg::PlatformPortEntry> getPlatformPortsByControllingPort(
    const std::map<int32_t, cfg::PlatformPortEntry>& platformPorts,
    PortID controllingPort) {
  std::vector<cfg::PlatformPortEntry> ports;
  int32_t controllingPortID = static_cast<int32_t>(controllingPort);
  for (const auto& port : platformPorts) {
    if (*port.second.mapping_ref()->controllingPort_ref() ==
        controllingPortID) {
      ports.push_back(port.second);
    }
  }
  return ports;
}

std::map<std::string, phy::DataPlanePhyChip> getDataPlanePhyChips(
    const cfg::PlatformPortEntry& port,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap,
    std::optional<phy::DataPlanePhyChipType> chipType) {
  std::map<std::string, phy::DataPlanePhyChip> chips;

  // if not specifying chipType, we return all the chips for such port
  std::vector<phy::DataPlanePhyChipType> types;
  if (chipType) {
    types.push_back(*chipType);
  } else {
    types.push_back(phy::DataPlanePhyChipType::IPHY);
    types.push_back(phy::DataPlanePhyChipType::XPHY);
    types.push_back(phy::DataPlanePhyChipType::TRANSCEIVER);
  }

  for (auto type : types) {
    auto pins =
        getPinsByChipType(chipsMap, *port.mapping_ref()->pins_ref(), type);
    if (pins.empty()) {
      continue;
    }
    for (auto pin : pins) {
      checkPinType(pin, type);
      std::string chipName = getChipName(pin);
      if (auto itChip = chipsMap.find(chipName); itChip != chipsMap.end()) {
        chips.emplace(itChip->first, itChip->second);
      }
    }
  }
  return chips;
}

std::optional<TransceiverID> getTransceiverId(
    const cfg::PlatformPortEntry& port,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap) {
  auto transceiverChips = getDataPlanePhyChips(
      port, chipsMap, phy::DataPlanePhyChipType::TRANSCEIVER);
  // There should be no more than one transceiver associated with a port.
  CHECK_LE(transceiverChips.size(), 1);
  if (!transceiverChips.empty()) {
    return TransceiverID(*transceiverChips.begin()->second.physicalID_ref());
  }
  return std::nullopt;
}
} // namespace facebook::fboss::utility
