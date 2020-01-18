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
  if (chipsMap.find(pinConn.a.chip) != chipsMap.end()) {
    Pin temp;
    temp.set_end(pinConn.a);
    pins.push_back(temp);
    return;
  }

  auto zPinRef = pinConn.z_ref();
  if (!zPinRef) {
    return;
  }
  auto zPin = *zPinRef;
  if (zPin.getType() == Pin::Type::end) {
    if (chipsMap.find(zPin.get_end().chip) != chipsMap.end()) {
      Pin temp;
      temp.set_end(zPin.get_end());
      pins.push_back(temp);
      return;
    }
  } else if (zPin.getType() == Pin::Type::junction) {
    auto zJunction = zPin.get_junction();
    if (chipsMap.find(zJunction.system.chip) != chipsMap.end()) {
      Pin temp;
      temp.set_junction(zJunction);
      pins.push_back(temp);
      return;
    }
    for (const auto& connection : zPin.get_junction().line) {
      recurseGetPinsByChipType(connection, chipsMap, pins);
    }
  }
}

std::vector<Pin> getPinsByChipType(
    const std::vector<DataPlanePhyChip>& chips,
    const std::vector<PinConnection>& pinConns,
    DataPlanePhyChipType type) {
  // First get all the expected chip type into a map to make search easier
  std::map<std::string, DataPlanePhyChip> chipsMap;
  for (const auto chip : chips) {
    if (chip.type == type) {
      chipsMap[chip.name] = chip;
    }
  }
  // use the recurse function to get all the expected chip pin
  std::vector<Pin> results;
  for (const auto& pinConn : pinConns) {
    recurseGetPinsByChipType(pinConn, chipsMap, results);
  }
  return results;
}
} // unnamed namespace

namespace facebook::fboss::utility {

std::vector<phy::PinID> getTransceiverLanes(
    const cfg::PlatformPortEntry& port,
    const std::vector<phy::DataPlanePhyChip>& chips,
    std::optional<cfg::PortProfileID> profileID) {
  std::vector<phy::PinID> lanes;
  if (profileID) {
    auto itPortCfg = port.supportedProfiles.find(*profileID);
    if (itPortCfg == port.supportedProfiles.end()) {
      throw FbossError(
          "Port: ",
          port.mapping.name,
          " doesn't support profile:",
          apache::thrift::util::enumNameSafe(*profileID));
    }
    if (auto tcvr = itPortCfg->second.pins.transceiver_ref()) {
      for (const auto& pinCfg : *tcvr) {
        lanes.push_back(pinCfg.id);
      }
    }
  } else {
    // If it's not looking for the lanes list based on profile, return the
    // static lane mapping
    auto pins = getPinsByChipType(
        chips, port.mapping.pins, phy::DataPlanePhyChipType::TRANSCEIVER);
    // the return pins should always be PinID for transceiver
    for (auto pin : pins) {
      if (pin.getType() != phy::Pin::Type::end) {
        throw FbossError("Unsupported pin type for transceiver");
      }
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
          pinConnection.a.lane, *pinConnection.polaritySwap_ref());
    }
  }

  return polaritySwapMap;
}

std::map<int32_t, phy::LaneConfig> getIphyLaneConfigs(
    const std::vector<phy::PinConnection>& pinConnections,
    const std::vector<phy::PinConfig>& iphyPinConfigs) {
  std::map<int32_t, phy::LaneConfig> laneConfigs;
  auto iphyPolaritySwapMap = getIphyPolaritySwapMap(pinConnections);

  for (auto pinConfig : iphyPinConfigs) {
    phy::LaneConfig laneConfig;
    if (pinConfig.tx_ref()) {
      laneConfig.tx = *pinConfig.tx_ref();
    }
    auto polaritySwap = iphyPolaritySwapMap.find(pinConfig.id.lane);
    if (polaritySwap != iphyPolaritySwapMap.end()) {
      laneConfig.polaritySwap = polaritySwap->second;
    }
    laneConfigs.emplace(pinConfig.id.lane, laneConfig);
  }

  return laneConfigs;
}

std::map<int32_t, phy::PolaritySwap> getXphyLinePolaritySwapMap(
    const std::vector<phy::PinConnection>& pinConnections,
    const std::vector<phy::DataPlanePhyChip>& chips) {
  std::map<int32_t, phy::PolaritySwap> xphyPolaritySwapMap;
  const auto& xphyPinList =
      getPinsByChipType(chips, pinConnections, phy::DataPlanePhyChipType::XPHY);
  for (const auto& pin : xphyPinList) {
    if (pin.getType() != phy::Pin::Type::junction) {
      throw FbossError("Unsupported pin type for xphy");
    }
    for (const auto& connection : pin.get_junction().line) {
      if (auto pn = connection.polaritySwap_ref()) {
        xphyPolaritySwapMap.emplace(connection.a.lane, *pn);
      }
    }
  }
  return xphyPolaritySwapMap;
}

std::vector<phy::PinID> getOrderedIphyLanes(
    const cfg::PlatformPortEntry& port,
    const std::vector<phy::DataPlanePhyChip>& chips,
    std::optional<cfg::PortProfileID> profileID) {
  std::vector<phy::PinID> lanes;
  if (profileID) {
    auto itrPortCfg = port.supportedProfiles.find(*profileID);
    if (itrPortCfg == port.supportedProfiles.end()) {
      throw FbossError(
          "Port: ",
          port.mapping.name,
          " doesn't support profile:",
          apache::thrift::util::enumNameSafe(*profileID));
    }
    for (const auto& pinCfg : itrPortCfg->second.pins.iphy) {
      lanes.push_back(pinCfg.id);
    }
  } else {
    // If it's not looking for the lanes list based on profile, return the
    // static lane mapping
    auto pins = getPinsByChipType(
        chips, port.mapping.pins, phy::DataPlanePhyChipType::IPHY);
    // the return pins should always be PinID for transceiver
    for (auto pin : pins) {
      if (pin.getType() != phy::Pin::Type::end) {
        if (pin.getType() != phy::Pin::Type::end) {
          throw FbossError("Unsupported pin type for iphy");
        }
      }
      lanes.push_back(pin.get_end());
    }
  }
  if (lanes.empty()) {
    throw FbossError("Port: ", port.mapping.name, " doesn't have iphy lanes");
  }
  // Sort by lane
  std::sort(lanes.begin(), lanes.end(), [](const auto& lPin, const auto& rPin) {
    return lPin.lane < rPin.lane;
  });
  return lanes;
}

// Get subsidiary PortID list based on controlling port
std::map<PortID, std::vector<PortID>> getSubsidiaryPortIDs(
    const facebook::fboss::cfg::PlatformConfig& platformCfg) {
  std::map<PortID, std::vector<PortID>> results;
  if (auto platformPorts = platformCfg.platformPorts_ref()) {
    for (auto itPort : *platformPorts) {
      auto controlPortID =
          facebook::fboss::PortID(itPort.second.mapping.controllingPort);
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
  }
  return results;
}

std::vector<cfg::PlatformPortEntry> getPlatformPortsByControllingPort(
    const std::map<int32_t, cfg::PlatformPortEntry>& platformPorts,
    PortID controllingPort) {
  std::vector<cfg::PlatformPortEntry> ports;
  int32_t controllingPortID = static_cast<int32_t>(controllingPort);
  for (const auto& port : platformPorts) {
    if (port.second.mapping.controllingPort == controllingPortID) {
      ports.push_back(port.second);
    }
  }
  return ports;
}

std::optional<phy::DataPlanePhyChip> getXphyChip(
    const cfg::PlatformPortEntry& port,
    const std::vector<phy::DataPlanePhyChip>& chips) {
  auto pins = getPinsByChipType(
      chips, port.mapping.pins, phy::DataPlanePhyChipType::XPHY);
  if (pins.empty()) {
    return std::nullopt;
  }

  auto pin = pins.front();
  if (pin.getType() != phy::Pin::Type::junction) {
    throw FbossError("Unsupported pin type for xphy");
  }

  auto chipName = pin.get_junction().system.chip;
  auto chip = std::find_if(chips.begin(), chips.end(), [chipName](auto chip) {
    return chipName == chip.name;
  });
  CHECK(chip != chips.end());

  return *chip;
}

} // namespace facebook::fboss::utility
