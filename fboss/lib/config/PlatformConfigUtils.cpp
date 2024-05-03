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

bool recurseCheckPortOwnsChip(
    const PinConnection& pinConn,
    const DataPlanePhyChip& chip) {
  if (pinConn.get_a().get_chip() == chip.get_name()) {
    return true;
  }
  if (auto zPinRef = pinConn.z()) {
    if (zPinRef->getType() == Pin::Type::end) {
      if (zPinRef->get_end().get_chip() == chip.get_name()) {
        return true;
      }
    } else if (zPinRef->getType() == Pin::Type::junction) {
      const auto& zJunction = zPinRef->get_junction();
      if (zJunction.get_system().get_chip() == chip.get_name()) {
        return true;
      }
      for (const auto& connection : zJunction.get_line()) {
        // Because one port can only use one iphy, one xphy, one transceiver,
        // and if one PinConnection can't find this chip, you won't find
        // this chip on another PinConnection of the same port
        return recurseCheckPortOwnsChip(connection, chip);
      }
    }
  }
  return false;
}

void recurseGetPinsByChipType(
    const PinConnection& pinConn,
    const std::map<std::string, DataPlanePhyChip>& chipsMap,
    std::vector<Pin>& pins) {
  if (chipsMap.find(*pinConn.a()->chip()) != chipsMap.end()) {
    Pin temp;
    temp.end_ref() = *pinConn.a();
    pins.push_back(temp);
    return;
  }

  auto zPinRef = pinConn.z();
  if (!zPinRef) {
    return;
  }
  auto zPin = *zPinRef;
  if (zPin.getType() == Pin::Type::end) {
    if (chipsMap.find(*zPin.get_end().chip()) != chipsMap.end()) {
      Pin temp;
      temp.end_ref() = zPin.get_end();
      pins.push_back(temp);
      return;
    }
  } else if (zPin.getType() == Pin::Type::junction) {
    auto zJunction = zPin.get_junction();
    if (chipsMap.find(*zJunction.system()->chip()) != chipsMap.end()) {
      Pin temp;
      temp.junction_ref() = zJunction;
      pins.push_back(temp);
      return;
    }
    for (const auto& connection : *zPin.get_junction().line()) {
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
  for (const auto& itChip : chipsMap) {
    if (*itChip.second.type() == type) {
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
    return *pin.get_junction().system()->chip();
  }
  return *pin.get_end().chip();
}
} // unnamed namespace

namespace facebook::fboss::utility {

std::vector<phy::PinID> getTransceiverLanes(
    const cfg::PlatformPortEntry& port,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap,
    std::optional<cfg::PortProfileID> profileID) {
  std::vector<phy::PinID> lanes;
  if (profileID && profileID != cfg::PortProfileID::PROFILE_DEFAULT) {
    auto itPortCfg = port.supportedProfiles()->find(*profileID);
    if (itPortCfg == port.supportedProfiles()->end()) {
      throw FbossError(
          "Port: ",
          *port.mapping()->name(),
          " doesn't support profile:",
          apache::thrift::util::enumNameSafe(*profileID));
    }
    if (auto tcvr = itPortCfg->second.pins()->transceiver()) {
      for (const auto& pinCfg : *tcvr) {
        lanes.push_back(*pinCfg.id());
      }
    }
  } else {
    // If it's not looking for the lanes list based on profile, return the
    // static lane mapping
    auto pins = getPinsByChipType(
        chipsMap,
        *port.mapping()->pins(),
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
    if (pinConnection.polaritySwap()) {
      polaritySwapMap.emplace(
          *pinConnection.a()->lane(), *pinConnection.polaritySwap());
    }
  }

  return polaritySwapMap;
}

std::map<int32_t, phy::LaneConfig> getIphyLaneConfigs(
    const std::vector<phy::PinConfig>& iphyPinConfigs) {
  std::map<int32_t, phy::LaneConfig> laneConfigs;
  for (auto pinConfig : iphyPinConfigs) {
    if (!pinConfig.tx()) {
      continue;
    }
    phy::LaneConfig laneConfig;
    laneConfig.tx = *pinConfig.tx();
    laneConfigs.emplace(*pinConfig.id()->lane(), laneConfig);
  }

  return laneConfigs;
}

std::map<int32_t, phy::PolaritySwap> getXphyLinePolaritySwapMap(
    const cfg::PlatformPortEntry& platformPortEntry,
    cfg::PortProfileID profileID,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap,
    const phy::PortProfileConfig& portProfileConfig) {
  std::map<int32_t, phy::PolaritySwap> xphyPolaritySwapMap;

  auto profile = platformPortEntry.supportedProfiles()->find(profileID);
  if (profile == platformPortEntry.supportedProfiles()->end()) {
    throw FbossError(
        "Unsupported profile in getXphyLinePolaritySwapMap: ",
        apache::thrift::util::enumNameSafe(profileID));
  }

  if (!portProfileConfig.xphyLine()) {
    throw FbossError("No xphy line info in portProfileConfig");
  }

  auto numLanes = portProfileConfig.xphyLine()->numLanes();

  // If the profile has an Xphy pin config, use that to determine the number of
  //   lanes, otherwise use the default pin count in the platformPortEntry

  // If the profile has an Xphy pin config, use that to determine the mapping.
  // If there's no profile pin config, or the profile doesn't contain any
  //   polarity swap info, get the polarity swap from the default pin mapping
  //   from the platformPortEntry
  auto profilePins = profile->second.pins()->xphyLine();
  if (profilePins) {
    for (const auto& pin : *profilePins) {
      if (pin.polaritySwap()) {
        xphyPolaritySwapMap[*pin.id()->lane()] = *pin.polaritySwap();
      }
    }
  }
  if (xphyPolaritySwapMap.empty()) {
    const auto& xphyPinList = getPinsByChipType(
        chipsMap,
        *platformPortEntry.mapping()->pins(),
        phy::DataPlanePhyChipType::XPHY);
    for (const auto& pin : xphyPinList) {
      checkPinType(pin, phy::DataPlanePhyChipType::XPHY);
      for (const auto& connection : *pin.get_junction().line()) {
        if (auto pn = connection.polaritySwap()) {
          xphyPolaritySwapMap.emplace(*connection.a()->lane(), *pn);
        }
      }
    }
  }
  if (!(xphyPolaritySwapMap.size() == 0 ||
        xphyPolaritySwapMap.size() == *numLanes)) {
    throw FbossError(
        "Computed polarity swap map contained ",
        xphyPolaritySwapMap.size(),
        " lanes, but either 0 or ",
        *numLanes,
        " lanes are required.");
  }
  return xphyPolaritySwapMap;
}

std::vector<phy::PinID> getOrderedIphyLanes(
    const cfg::PlatformPortEntry& port,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap,
    std::optional<cfg::PortProfileID> profileID) {
  std::vector<phy::PinID> lanes;
  if (profileID && profileID != cfg::PortProfileID::PROFILE_DEFAULT) {
    auto itrPortCfg = port.supportedProfiles()->find(*profileID);
    if (itrPortCfg == port.supportedProfiles()->end()) {
      throw FbossError(
          "Port: ",
          *port.mapping()->name(),
          " doesn't support profile:",
          apache::thrift::util::enumNameSafe(*profileID));
    }
    for (const auto& pinCfg : *itrPortCfg->second.pins()->iphy()) {
      lanes.push_back(*pinCfg.id());
    }
  } else {
    // If it's not looking for the lanes list based on profile, return the
    // static lane mapping
    auto pins = getPinsByChipType(
        chipsMap, *port.mapping()->pins(), phy::DataPlanePhyChipType::IPHY);
    // the return pins should always be PinID for transceiver
    for (auto pin : pins) {
      checkPinType(pin, phy::DataPlanePhyChipType::IPHY);
      lanes.push_back(pin.get_end());
    }
  }
  if (lanes.empty()) {
    throw FbossError(
        "Port: ", *port.mapping()->name(), " doesn't have iphy lanes");
  }
  // Sort by lane
  std::sort(lanes.begin(), lanes.end(), [](const auto& lPin, const auto& rPin) {
    return *lPin.lane() < *rPin.lane();
  });
  return lanes;
}

// Get subsidiary PortID list based on controlling port
std::map<PortID, std::vector<PortID>> getSubsidiaryPortIDs(
    const std::map<int32_t, cfg::PlatformPortEntry>& platformPorts) {
  std::map<PortID, std::vector<PortID>> results;
  for (auto itPort : platformPorts) {
    auto controlPortID =
        facebook::fboss::PortID(*itPort.second.mapping()->controllingPort());
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
    if (*port.second.mapping()->controllingPort() == controllingPortID) {
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
    auto pins = getPinsByChipType(chipsMap, *port.mapping()->pins(), type);
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
    return TransceiverID(*transceiverChips.begin()->second.physicalID());
  }
  return std::nullopt;
}

std::vector<cfg::PlatformPortEntry> getPlatformPortsByChip(
    const std::map<int32_t, cfg::PlatformPortEntry>& platformPorts,
    const phy::DataPlanePhyChip& chip) {
  std::vector<cfg::PlatformPortEntry> ports;
  for (const auto& idAndPort : platformPorts) {
    for (const auto& connection : idAndPort.second.get_mapping().get_pins()) {
      if (recurseCheckPortOwnsChip(connection, chip)) {
        ports.push_back(idAndPort.second);
        break;
      }
    }
  }
  return ports;
}
} // namespace facebook::fboss::utility
