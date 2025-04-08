/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/logging/xlog.h>
#include <re2/re2.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/FbossError.h"

DEFINE_string(
    platform_mapping_override_path,
    "",
    "The path to the Platform Mapping JSON file");

DEFINE_bool(
    multi_npu_platform_mapping,
    false,
    "use multi-npu platform mapping for applicable platforms");

DEFINE_int32(platform_mapping_profile, 0, "Platform mapping profile");

namespace {
constexpr auto kFbossPortNameRegex = "eth(\\d+)/(\\d+)/(\\d+)";
const re2::RE2 portNameRegex(kFbossPortNameRegex);
} // namespace

namespace facebook::fboss {

cfg::PlatformPortConfigOverrideFactor buildPlatformPortConfigOverrideFactor(
    const TransceiverInfo& transceiverInfo) {
  cfg::PlatformPortConfigOverrideFactor factor;
  if (auto cable = transceiverInfo.tcvrState()->cable();
      cable && cable->length()) {
    factor.cableLengths() = {*cable->length()};
  }
  if (auto settings = transceiverInfo.tcvrState()->settings()) {
    if (auto mediaInterfaces = settings->mediaInterface();
        mediaInterfaces && !mediaInterfaces->empty()) {
      // Use the first lane mediaInterface
      factor.mediaInterfaceCode() = *(*mediaInterfaces)[0].code();
    }
  }
  if (auto interface =
          transceiverInfo.tcvrState()->transceiverManagementInterface()) {
    factor.transceiverManagementInterface() = *interface;
  }
  return factor;
}

bool PlatformPortProfileConfigMatcher::matchOverrideWithFactor(
    const cfg::PlatformPortConfigOverrideFactor& factor) {
  if (auto overridePorts = factor.ports()) {
    if (!portID_.has_value() ||
        std::find(
            overridePorts->begin(),
            overridePorts->end(),
            static_cast<int>(portID_.value())) == overridePorts->end()) {
      return false;
    }
  }
  if (auto overrideProfiles = factor.profiles()) {
    if (std::find(
            overrideProfiles->begin(), overrideProfiles->end(), profileID_) ==
        overrideProfiles->end()) {
      return false;
    }
  }
  if (auto overrideCableLength = factor.cableLengths()) {
    if (!portConfigOverrideFactor_ ||
        !portConfigOverrideFactor_->cableLengths()) {
      return false;
    }
    for (auto cableLength : *portConfigOverrideFactor_->cableLengths()) {
      if (std::find(
              overrideCableLength->begin(),
              overrideCableLength->end(),
              cableLength) == overrideCableLength->end()) {
        return false;
      }
    }
  }
  if (auto overrideMediaInterfaceCode = factor.mediaInterfaceCode()) {
    if (!portConfigOverrideFactor_ ||
        portConfigOverrideFactor_->mediaInterfaceCode() !=
            overrideMediaInterfaceCode) {
      return false;
    }
  }
  if (auto overrideTransceiverManagementInterface =
          factor.transceiverManagementInterface()) {
    if (!portConfigOverrideFactor_ ||
        portConfigOverrideFactor_->transceiverManagementInterface() !=
            overrideTransceiverManagementInterface) {
      return false;
    }
  }
  if (auto overrideChips = factor.chips()) {
    if (!portConfigOverrideFactor_ || !portConfigOverrideFactor_->chips()) {
      return false;
    }
    for (const auto& chip : *portConfigOverrideFactor_->chips()) {
      if (std::find(overrideChips->begin(), overrideChips->end(), chip) ==
          overrideChips->end()) {
        return false;
      }
    }
  }
  return true;
}

bool PlatformPortProfileConfigMatcher::matchProfileWithFactor(
    const PlatformMapping* pm,
    const cfg::PlatformPortConfigFactor& factor) {
  if (folly::copy(factor.profileID().value()) != profileID_) {
    return false;
  }
  // if we dont have pimID but the factor does, try to get it from portID
  if (!pimID_.has_value() && factor.pimIDs().has_value() &&
      portID_.has_value()) {
    pimID_ = pm->getPimID(portID_.value());
  }
  if (auto pimIDs = factor.pimIDs()) {
    if (pimID_.has_value() && pimIDs->find(pimID_.value()) == pimIDs->end()) {
      return false;
    }
  }
  return true;
}

std::string PlatformPortProfileConfigMatcher::toString() const {
  std::string str = folly::to<std::string>(
      "profileID=", apache::thrift::util::enumNameSafe(profileID_));
  if (portID_) {
    str = folly::to<std::string>(str, ", portID=", *portID_);
  }
  if (pimID_) {
    str = folly::to<std::string>(str, ", pimID=", *pimID_);
  }
  if (portConfigOverrideFactor_) {
    str = folly::to<std::string>(
        str,
        ", portConfigOverrideFactor=",
        apache::thrift::SimpleJSONSerializer::serialize<std::string>(
            *portConfigOverrideFactor_));
  }
  return str;
}

PlatformMapping::PlatformMapping(const std::string& jsonPlatformMappingStr) {
  init(apache::thrift::SimpleJSONSerializer::deserialize<cfg::PlatformMapping>(
      jsonPlatformMappingStr));
}

PlatformMapping::PlatformMapping(const cfg::PlatformMapping& mapping) {
  init(mapping);
}

void PlatformMapping::init(const cfg::PlatformMapping& mapping) {
  platformPorts_ = std::move(*mapping.ports());
  platformSupportedProfiles_ = std::move(*mapping.platformSupportedProfiles());
  for (auto chip : *mapping.chips()) {
    chips_[*chip.name()] = chip;
  }
  if (auto portConfigOverrides = mapping.portConfigOverrides()) {
    portConfigOverrides_ = std::move(*portConfigOverrides);
  }
}

cfg::PlatformMapping PlatformMapping::toThrift() const {
  cfg::PlatformMapping newMapping;
  newMapping.ports() = this->platformPorts_;
  newMapping.platformSupportedProfiles() = this->platformSupportedProfiles_;
  for (const auto& nameChipPair : this->chips_) {
    newMapping.chips()->push_back(nameChipPair.second);
  }
  newMapping.portConfigOverrides() = this->portConfigOverrides_;
  return newMapping;
}

void PlatformMapping::merge(PlatformMapping* mapping) {
  for (auto port : mapping->platformPorts_) {
    platformPorts_.emplace(port.first, std::move(port.second));
    mergePortConfigOverrides(
        port.first, mapping->getPortConfigOverrides(port.first));
  }
  mapping->platformPorts_.clear();

  for (const auto& incomingProfile : mapping->platformSupportedProfiles_) {
    mergePlatformSupportedProfile(incomingProfile);
  }
  mapping->platformSupportedProfiles_.clear();

  for (auto chip : mapping->chips_) {
    chips_.emplace(chip.first, std::move(chip.second));
  }
  mapping->chips_.clear();
}

void PlatformMapping::mergePlatformSupportedProfile(
    cfg::PlatformPortProfileConfigEntry incomingProfile) {
  for (auto& currentProfile : platformSupportedProfiles_) {
    auto currentFactor = currentProfile.factor();
    auto incomingFactor = incomingProfile.factor();
    // Merge factors if profileID and profile config are the same
    if (incomingFactor->profileID() == currentFactor->profileID()) {
      if (incomingProfile.profile() == currentProfile.profile()) {
        // if our currect factor has no pim restriction (supports all pims),
        // skip merging
        if (!currentFactor->pimIDs().has_value()) {
          continue;
        }
        if (auto incomingPims = incomingFactor->pimIDs()) {
          currentFactor->pimIDs()->insert(
              incomingPims->begin(), incomingPims->end());
        }
        // found where to merge factors, we can return here
        return;
      } else if (
          currentFactor->pimIDs().has_value() &&
          incomingFactor->pimIDs().has_value()) {
        // If profileIDs are the same but the profile configs are different,
        // then the pimIDs better be mutually exclusive, otherwise there is no
        // reasonable merge
        std::vector<int> intersection;
        std::set_intersection(
            currentFactor->pimIDs()->begin(),
            currentFactor->pimIDs()->end(),
            incomingFactor->pimIDs()->begin(),
            incomingFactor->pimIDs()->end(),
            std::back_inserter(intersection));
        if (intersection.size() != 0) {
          throw FbossError(
              "Supported profiles with different configs are not mutualy exclusive");
        }
      }
    }
  }
  // Was not able to merge entry so create new one
  platformSupportedProfiles_.push_back(incomingProfile);
}

int PlatformMapping::getPimID(PortID portID) const {
  auto itPlatformPort = platformPorts_.find(portID);
  if (itPlatformPort == platformPorts_.end()) {
    throw FbossError("Unrecoganized port:", portID);
  }
  return getPimID(itPlatformPort->second);
}

int PlatformMapping::getPimID(
    const cfg::PlatformPortEntry& platformPort) const {
  int pimID = 0;
  auto& portName = platformPort.mapping().value().name().value();
  re2::RE2 portNameRe(kFbossPortNameRegex);
  if (!re2::RE2::FullMatch(portName, portNameRe, &pimID)) {
    throw FbossError(
        "Invalid port name: ",
        portName,
        " for port id: ",
        folly::copy(platformPort.mapping().value().id().value()));
  }
  return pimID;
}

const phy::DataPlanePhyChip& PlatformMapping::getPortIphyChip(
    PortID portID) const {
  auto itPlatformPort = platformPorts_.find(portID);
  if (itPlatformPort == platformPorts_.end()) {
    throw FbossError("Unrecoganized port:", portID);
  }
  const auto& coreName =
      itPlatformPort->second.mapping()->pins()[0].a()->chip().value();
  return chips_.at(coreName);
}

cfg::PortSpeed PlatformMapping::getPortMaxSpeed(PortID portID) const {
  auto itPlatformPort = platformPorts_.find(portID);
  if (itPlatformPort == platformPorts_.end()) {
    throw FbossError("Unrecoganized port:", portID);
  }

  cfg::PortSpeed maxSpeed{cfg::PortSpeed::DEFAULT};
  for (const auto& profile : *itPlatformPort->second.supportedProfiles()) {
    if (auto profileConfig = getPortProfileConfig(
            PlatformPortProfileConfigMatcher(profile.first, portID))) {
      if (static_cast<int>(maxSpeed) <
          static_cast<int>(*profileConfig->speed())) {
        maxSpeed = *profileConfig->speed();
      }
    }
  }
  return maxSpeed;
}

cfg::Scope PlatformMapping::getPortScope(PortID portID) const {
  auto itPlatformPort = platformPorts_.find(portID);
  if (itPlatformPort == platformPorts_.end()) {
    throw FbossError("Unrecoganized port:", portID);
  }
  return *itPlatformPort->second.mapping()->scope();
}

std::vector<phy::PinConfig> PlatformMapping::getPortIphyPinConfigs(
    PlatformPortProfileConfigMatcher matcher) const {
  std::optional<phy::DataPlanePhyChip> chip;
  if (auto overrideFactor = matcher.getPortConfigOverrideFactorIf()) {
    if (auto chips = overrideFactor->chips(); chips && !chips->empty()) {
      chip = (*chips)[0];
    }
  }
  auto portID = matcher.getPortIDIf();
  auto profileID = matcher.getProfileID();

  if (!portID.has_value() && !chip.has_value()) {
    throw FbossError("getPortIphyPinConfigs missing portID/chip match factor");
  }

  if (portID.has_value()) {
    const auto& platformPortConfig =
        getPlatformPortConfig(portID.value(), profileID);
    const auto& iphyCfg = *platformPortConfig.pins()->iphy();
    // Check whether there's an override
    for (const auto& portConfigOverride : portConfigOverrides_) {
      if (!portConfigOverride.pins().has_value()) {
        // The override is not about Iphy pin configs. Skip
        continue;
      }
      if (matcher.matchOverrideWithFactor(*portConfigOverride.factor())) {
        auto overrideIphy = *portConfigOverride.pins()->iphy();
        if (!overrideIphy.empty()) {
          // make sure the override iphy config size == iphyCfg or override
          // size == 1, in which case we use the same override for all lanes
          if (overrideIphy.size() != iphyCfg.size() &&
              overrideIphy.size() != 1) {
            throw FbossError(
                "Port ",
                portID.value(),
                ", profile ",
                apache::thrift::util::enumNameSafe(profileID),
                " has mismatched override iphy lane size:",
                overrideIphy.size(),
                ", expected size: ",
                iphyCfg.size());
          }

          // We need to update the override with the correct PinID for such port
          // Both default iphyCfg and override iphyCfg are in order by lanes.
          std::vector<phy::PinConfig> newOverrideIphy;
          for (int i = 0; i < iphyCfg.size(); i++) {
            phy::PinConfig pinCfg;
            pinCfg.id() = *iphyCfg.at(i).id();
            // Default to the first entry if we run out
            const auto& override =
                overrideIphy.at(i < overrideIphy.size() ? i : 0);
            if (auto tx = override.tx()) {
              pinCfg.tx() = *tx;
            }
            newOverrideIphy.push_back(pinCfg);
          }
          return newOverrideIphy;
        }
      }
    }
    // otherwise, we just need to return iphy config directly
    return iphyCfg;
  } else {
    for (const auto& portConfigOverride : portConfigOverrides_) {
      if (!portConfigOverride.pins().has_value()) {
        // The override is not about Iphy pin configs. Skip
        continue;
      }
      if (matcher.matchOverrideWithFactor(*portConfigOverride.factor())) {
        return *portConfigOverride.pins()->iphy();
      }
    }
  }
  throw FbossError("No iphy pins found for matcher", matcher.toString());
}

std::optional<std::vector<phy::PinConfig>>
PlatformMapping::getPortTransceiverPinConfigs(
    PlatformPortProfileConfigMatcher matcher) const {
  auto portID = matcher.getPortIDIf();
  auto profileID = matcher.getProfileID();
  if (!portID.has_value()) {
    throw FbossError("getPortIphyPinConfigs miss portID match factor");
  }
  const auto& platformPortConfig =
      getPlatformPortConfig(portID.value(), profileID);

  // no transceiver pin overrides
  if (auto transceiverPins = platformPortConfig.pins()->transceiver()) {
    return *transceiverPins;
  }
  return std::nullopt;
}

std::set<uint8_t> PlatformMapping::getTransceiverHostLanes(
    PlatformPortProfileConfigMatcher matcher) const {
  auto portID = matcher.getPortIDIf();

  if (!portID.has_value()) {
    throw FbossError("getTransceiverHostLanes miss portID match factor");
  }

  std::set<uint8_t> tcvrHostLanes;
  auto pinConfigs = getPortTransceiverPinConfigs(matcher);
  if (!pinConfigs || pinConfigs->empty()) {
    throw FbossError("Can't find tcvr pinConfigs for portId ", *portID);
  }
  for (auto pin : *pinConfigs) {
    uint8_t lane = *pin.id()->lane();
    tcvrHostLanes.insert(lane);
  }
  return tcvrHostLanes;
}

int PlatformMapping::getTransceiverIdFromSwPort(PortID swPort) const {
  const auto& platformPorts = getPlatformPorts();
  const auto& chips = getChips();

  auto platformPortItr = platformPorts.find(static_cast<int32_t>(swPort));
  if (platformPortItr == platformPorts.end()) {
    throw FbossError("Can't find Platform Port for portId ", swPort);
  }

  auto tcvrID = utility::getTransceiverId(platformPortItr->second, chips);
  if (!tcvrID.has_value()) {
    throw FbossError("Can't find Tcvr ID for portId ", swPort);
  }

  return tcvrID.value();
}

std::vector<PortID> PlatformMapping::getSwPortListFromTransceiverId(
    int tcvrId) const {
  std::optional<phy::DataPlanePhyChip> tcvrChip;
  for (const auto& chip : getChips()) {
    if (*chip.second.type() == phy::DataPlanePhyChipType::TRANSCEIVER &&
        *chip.second.physicalID() == tcvrId) {
      tcvrChip = chip.second;
      break;
    }
  }
  if (!tcvrChip) {
    throw FbossError(
        "Can't find transceiver: ", tcvrId, " from PlatformMapping");
  }

  const auto& platformPorts =
      utility::getPlatformPortsByChip(getPlatformPorts(), *tcvrChip);
  if (platformPorts.empty()) {
    throw FbossError("Can't find platformPorts for transceiver: ", tcvrId);
  }

  std::vector<PortID> swPorts;
  for (auto platformPort : platformPorts) {
    swPorts.emplace_back(*platformPort.mapping()->id());
  }
  return swPorts;
}

std::vector<phy::PinConfig> PlatformMapping::getPortXphySidePinConfigs(
    PlatformPortProfileConfigMatcher matcher,
    phy::Side side) const {
  auto portID = matcher.getPortIDIf();
  auto profileID = matcher.getProfileID();
  if (!portID.has_value()) {
    throw FbossError("getPortXphySidePinConfigs miss portID match factor");
  }
  const auto& platformPortConfig =
      getPlatformPortConfig(portID.value(), profileID);
  auto xphySideOptional = side == phy::Side::SYSTEM
      ? platformPortConfig.pins()->xphySys()
      : platformPortConfig.pins()->xphyLine();
  if (!xphySideOptional.has_value()) {
    return std::vector<phy::PinConfig>();
  }
  const auto& xphySideCfg = xphySideOptional.value();
  // Check whether there's an override
  for (const auto& portConfigOverride : portConfigOverrides_) {
    if (!portConfigOverride.pins().has_value()) {
      // The override is not about pin configs. Skip
      continue;
    }
    auto overrideXphySideOptional = side == phy::Side::SYSTEM
        ? portConfigOverride.pins()->xphySys()
        : portConfigOverride.pins()->xphyLine();
    if (!overrideXphySideOptional.has_value()) {
      // The override is not about xphy configs. Skip
      continue;
    }
    if (matcher.matchOverrideWithFactor(*portConfigOverride.factor())) {
      auto overrideXphySideCfg = overrideXphySideOptional.value();
      if (!overrideXphySideCfg.empty()) {
        // make sure the override xphy config size == xphySideCfg or
        // override size == 1, in which case we use the same override for all
        // lanes
        if (overrideXphySideCfg.size() != xphySideCfg.size() &&
            overrideXphySideCfg.size() != 1) {
          throw FbossError(
              "Port ",
              portID.value(),
              ", profile ",
              apache::thrift::util::enumNameSafe(profileID),
              " has mismatched override xphy side lane size:",
              overrideXphySideCfg.size(),
              ", expected size: ",
              xphySideCfg.size());
        }

        // We need to update the override with the correct PinID for such port
        // Both default xphySideCfg and override xphySideCfg are in order by
        // lanes.
        std::vector<phy::PinConfig> newOverrideXphySide;
        for (int i = 0; i < xphySideCfg.size(); i++) {
          phy::PinConfig pinCfg;
          *pinCfg.id() = *xphySideCfg.at(i).id();
          // Default to the first entry if we run out
          const auto& override =
              overrideXphySideCfg.at(i < overrideXphySideCfg.size() ? i : 0);
          if (auto tx = override.tx()) {
            pinCfg.tx() = *tx;
          }
          newOverrideXphySide.push_back(pinCfg);
        }
        return newOverrideXphySide;
      }
    }
  }
  // otherwise, we just need to return xphy side config directly
  return xphySideCfg;
}

phy::PortPinConfig PlatformMapping::getPortXphyPinConfig(
    PlatformPortProfileConfigMatcher matcher) const {
  phy::PortPinConfig newPortPinConfig;
  newPortPinConfig.xphySys() =
      getPortXphySidePinConfigs(matcher, phy::Side::SYSTEM);
  newPortPinConfig.xphyLine() =
      getPortXphySidePinConfigs(matcher, phy::Side::LINE);
  if (auto transceivers = getPortTransceiverPinConfigs(matcher)) {
    newPortPinConfig.transceiver() = *transceivers;
  }
  return newPortPinConfig;
}

const std::optional<phy::PortProfileConfig>
PlatformMapping::getPortProfileConfig(
    PlatformPortProfileConfigMatcher profileMatcher) const {
  for (const auto& portConfigOverride : portConfigOverrides_) {
    if (!portConfigOverride.portProfileConfig().has_value()) {
      // The override is not about portProfileConfig. Skip
      continue;
    }
    if (profileMatcher.matchOverrideWithFactor(*portConfigOverride.factor())) {
      return *portConfigOverride.portProfileConfig();
    }
  }
  for (auto& supportedProfile : platformSupportedProfiles_) {
    if (profileMatcher.matchProfileWithFactor(
            this, supportedProfile.factor().value())) {
      return supportedProfile.profile().value();
    }
  }
  XLOGF(
      INFO,
      "Could not find profile with matcher={}",
      profileMatcher.toString());
  return std::nullopt;
}

std::vector<cfg::PlatformPortConfigOverride>
PlatformMapping::getPortConfigOverrides(int32_t port) const {
  std::vector<cfg::PlatformPortConfigOverride> overrides;
  for (const auto& portConfigOverride : portConfigOverrides_) {
    if (auto portList = portConfigOverride.factor()->ports()) {
      if (std::find(portList->begin(), portList->end(), port) !=
          portList->end()) {
        overrides.push_back(portConfigOverride);
      }
    } else if (auto chipList = portConfigOverride.factor()->chips()) {
      auto chip = getPortIphyChip(PortID(port));
      if (std::find(chipList->begin(), chipList->end(), chip) !=
          chipList->end()) {
        overrides.push_back(portConfigOverride);
      }
    } else {
      // If factor.ports is empty, which means such override apply for all
      // ports
      overrides.push_back(portConfigOverride);
    }
  }
  return overrides;
}

void PlatformMapping::mergePortConfigOverrides(
    int32_t port,
    std::vector<cfg::PlatformPortConfigOverride> overrides) {
  for (auto& portOverrides : overrides) {
    int numMismatch = 0;
    for (auto& curOverride : portConfigOverrides_) {
      if (portOverrides.pins() != curOverride.pins() ||
          portOverrides.factor()->profiles() !=
              curOverride.factor()->profiles() ||
          portOverrides.factor()->cableLengths() !=
              curOverride.factor()->cableLengths() ||
          portOverrides.factor()->mediaInterfaceCode() !=
              curOverride.factor()->mediaInterfaceCode() ||
          portOverrides.factor()->chips() != curOverride.factor()->chips()) {
        numMismatch++;
        continue;
      }

      auto portList = portOverrides.factor()->ports();
      auto curPortList = curOverride.factor()->ports();
      if (portList && curPortList) {
        curPortList->push_back(port);
      }
    }
    // if none of the existing override matches, add this override directly
    if (numMismatch == portConfigOverrides_.size()) {
      if (auto portList = portOverrides.factor()->ports()) {
        cfg::PlatformPortConfigOverride newOverride;
        *newOverride.factor() = *portOverrides.factor();
        newOverride.factor()->ports() = {port};
        newOverride.pins().copy_from(portOverrides.pins());
        portConfigOverrides_.push_back(newOverride);
      } else {
        portConfigOverrides_.push_back(portOverrides);
      }
    }
  }
}

const PortID PlatformMapping::getPortID(const std::string& portName) const {
  for (const auto& platPortEntry : platformPorts_) {
    if (*platPortEntry.second.mapping()->name() == portName) {
      return PortID(*platPortEntry.second.mapping()->id());
    }
  }
  throw FbossError("No PlatformPortEntry found for portName: ", portName);
}

std::optional<std::string> PlatformMapping::getPortNameByPortId(
    PortID portId) const {
  const auto& platformPorts = getPlatformPorts();
  int32_t portIdInt = static_cast<int32_t>(portId);
  if (platformPorts.find(portIdInt) != platformPorts.end()) {
    return *platformPorts.at(portIdInt).mapping()->name();
  }
  return std::nullopt;
}

std::optional<int32_t> PlatformMapping::getVirtualDeviceID(
    const std::string& portName) const {
  for (const auto& platPortEntry : platformPorts_) {
    if (*platPortEntry.second.mapping()->name() == portName) {
      return platPortEntry.second.mapping()->virtualDeviceId()
          ? *platPortEntry.second.mapping()->virtualDeviceId()
          : std::optional<int32_t>();
    }
  }

  throw FbossError("No PlatformPortEntry found for portName: ", portName);
}

const cfg::PlatformPortConfig& PlatformMapping::getPlatformPortConfig(
    PortID id,
    cfg::PortProfileID profileID) const {
  auto itPlatformPort = platformPorts_.find(id);
  if (itPlatformPort == platformPorts_.end()) {
    throw FbossError("No PlatformPortEntry found for port ", id);
  }

  auto& supportedProfiles = *itPlatformPort->second.supportedProfiles();
  auto platformPortConfig = supportedProfiles.find(profileID);
  if (platformPortConfig == supportedProfiles.end()) {
    throw FbossError(
        "No speed profile with id ",
        apache::thrift::util::enumNameSafe(profileID),
        " found in PlatformPortEntry for port ",
        id);
  }
  return platformPortConfig->second;
}

std::map<phy::DataPlanePhyChip, std::vector<phy::PinConfig>>
PlatformMapping::getCorePinMapping(const std::vector<cfg::Port>& ports) const {
  std::map<phy::DataPlanePhyChip, std::vector<phy::PinConfig>> corePinMapping;
  const auto& platformPorts = getPlatformPorts();
  for (auto& port : ports) {
    auto portID = folly::copy(port.logicalID().value());
    if (platformPorts.find(portID) == platformPorts.end()) {
      throw FbossError("Could not find platform port with id ", portID);
    }
    auto& platformPortEntry = platformPorts.at(portID);
    auto profileID = folly::copy(port.profileID().value());
    if (portID !=
        folly::copy(platformPortEntry.mapping()->controllingPort().value())) {
      continue;
    }
    const auto& chip = getPortIphyChip(PortID(portID));
    cfg::PlatformPortConfigOverrideFactor factor;
    factor.chips() = {chip};
    corePinMapping[chip] = getPortIphyPinConfigs(
        PlatformPortProfileConfigMatcher(profileID, std::nullopt, factor));
  }
  return corePinMapping;
}

const cfg::PlatformPortEntry& PlatformMapping::getPlatformPort(
    int32_t portId) const {
  auto entry = platformPorts_.find(portId);
  if (entry != platformPorts_.end()) {
    return entry->second;
  }
  throw FbossError("No PlatformMapping entry for port ", portId);
}

std::map<std::string, phy::DataPlanePhyChip>
PlatformMapping::getPortDataplaneChips(
    PlatformPortProfileConfigMatcher matcher) const {
  std::map<std::string, phy::DataPlanePhyChip> chips;
  const auto& pins = getPortXphyPinConfig(matcher);
  auto allChips = getChips();

  auto addChips = [&allChips, &chips](const auto& pins) {
    for (auto& pin : pins) {
      auto chip = *pin.id()->chip();
      chips[chip] = allChips[chip];
    }
  };

  addChips(*pins.iphy());
  if (auto xphySys = pins.xphySys()) {
    addChips(*xphySys);
  }
  if (auto xphyLine = pins.xphyLine()) {
    addChips(*xphyLine);
  }
  if (auto transceiver = pins.transceiver()) {
    addChips(*transceiver);
  }

  return chips;
}

cfg::PortProfileID PlatformMapping::getProfileIDBySpeed(
    PortID portID,
    cfg::PortSpeed speed) const {
  auto profile = getProfileIDBySpeedIf(portID, speed);
  if (!profile.has_value()) {
    throw FbossError(
        "Platform port ",
        portID,
        " has no profile for speed ",
        apache::thrift::util::enumNameSafe(speed));
  }
  return profile.value();
}

std::optional<cfg::PortProfileID> PlatformMapping::getProfileIDBySpeedIf(
    PortID portID,
    cfg::PortSpeed speed) const {
  if (speed == cfg::PortSpeed::DEFAULT) {
    return cfg::PortProfileID::PROFILE_DEFAULT;
  }

  const auto& platformPortEntry = getPlatformPort(portID);
  for (const auto& profile : *platformPortEntry.supportedProfiles()) {
    auto profileID = profile.first;
    if (auto profileCfg = getPortProfileConfig(
            PlatformPortProfileConfigMatcher(profileID, portID))) {
      if (*profileCfg->speed() == speed) {
        return profileID;
      }
    } else {
      throw FbossError(
          "Platform port ",
          portID,
          " has invalid profile ",
          apache::thrift::util::enumNameSafe(profileID));
    }
  }
  XLOG(DBG2) << "Can't find supported profile for port=" << portID
             << ", speed=" << apache::thrift::util::enumNameSafe(speed);
  return std::nullopt;
}

/*
 * getAllPortProfiles
 *
 * Returns a map of all port names in the system to the list of supported port
 * profile ids for that port.
 */
std::map<std::string, std::vector<cfg::PortProfileID>>
PlatformMapping::getAllPortProfiles() const {
  std::map<std::string, std::vector<cfg::PortProfileID>> portProfileIds;

  for (auto& platformPort : platformPorts_) {
    auto& portName = *platformPort.second.mapping()->name();
    auto& portProfiles = *platformPort.second.supportedProfiles();
    std::vector<cfg::PortProfileID> profiles;
    for (auto& profile : portProfiles) {
      profiles.push_back(profile.first);
    }
    portProfileIds[portName] = profiles;
  }
  return portProfileIds;
}

/*
 * getPortProfileFromLinkProperties
 *
 * Returns the Port Profile ID based on the link properties. The following 5
 * link properties can uniquely identify a Port Profile:
 *   - Speed (100G/200G/...)
 *   - Number of Host Lanes
 *   - IP Modulation (NRZ/PAM4)
 *   - FEC (RS528/RS544/RS544_2N/...)
 *   - Transmitter Media (OPTICAL/COPPER/...)
 */
std::vector<cfg::PortProfileID>
PlatformMapping::getPortProfileFromLinkProperties(
    cfg::PortSpeed speed,
    uint16_t numLanes,
    phy::IpModulation modulation,
    phy::FecMode fec,
    std::optional<TransmitterTechnology> medium) const {
  std::vector<cfg::PortProfileID> profiles;

  for (auto& supportedProfile : platformSupportedProfiles_) {
    auto& profile = supportedProfile.profile().value();
    if (profile.speed().value() == speed &&
        profile.iphy().value().numLanes().value() == numLanes &&
        profile.iphy().value().modulation().value() == modulation &&
        profile.iphy().value().fec().value() == fec) {
      if (medium.has_value() && profile.iphy().value().medium().has_value()) {
        if (medium.value() != profile.iphy().value().medium().value()) {
          if (medium.value() == TransmitterTechnology::OPTICAL &&
              profile.iphy().value().medium().value() ==
                  TransmitterTechnology::BACKPLANE) {
            profiles.push_back(
                supportedProfile.factor().value().profileID().value());
          }
          continue;
        }
      }
      profiles.push_back(supportedProfile.factor().value().profileID().value());
    }
  }
  return profiles;
}

std::vector<PortID> PlatformMapping::getPlatformPorts(
    cfg::PortType portType) const {
  std::vector<PortID> portIds;
  for (const auto& port : getPlatformPorts()) {
    auto portID = PortID(port.first);
    const auto& platformPort = port.second;
    if (platformPort.mapping()->portType() == portType) {
      portIds.push_back(portID);
    }
  }
  return portIds;
}

} // namespace facebook::fboss
