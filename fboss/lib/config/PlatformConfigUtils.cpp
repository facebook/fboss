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

namespace facebook {
namespace fboss {
namespace utility {

std::vector<phy::PinID> getTransceiverLanes(
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
    if (auto tcvr = itrPortCfg->second.pins.transceiver_ref()) {
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
} // namespace utility
} // namespace fboss
} // namespace facebook
