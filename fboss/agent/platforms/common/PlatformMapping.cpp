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

#include <folly/logging/xlog.h>
#include <re2/re2.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/FbossError.h"

namespace {
constexpr auto kFbossPortNameRegex = "eth(\\d+)/(\\d+)/(\\d+)";
const re2::RE2 portNameRegex(kFbossPortNameRegex);
} // namespace

namespace facebook {
namespace fboss {

cfg::PlatformPortConfigOverrideFactor buildPlatformPortConfigOverrideFactor(
    const TransceiverInfo& transceiverInfo) {
  cfg::PlatformPortConfigOverrideFactor factor;
  if (auto cable = transceiverInfo.cable_ref(); cable && cable->length_ref()) {
    factor.cableLengths_ref() = {*cable->length_ref()};
  }
  if (auto settings = transceiverInfo.settings_ref()) {
    if (auto mediaInterfaces = settings->mediaInterface_ref();
        mediaInterfaces && !mediaInterfaces->empty()) {
      // Use the first lane mediaInterface
      factor.mediaInterfaceCode_ref() = *(*mediaInterfaces)[0].code_ref();
    }
  }
  if (auto interface = transceiverInfo.transceiverManagementInterface_ref()) {
    factor.transceiverManagementInterface_ref() = *interface;
  }
  return factor;
}

bool PlatformPortProfileConfigMatcher::matchOverrideWithFactor(
    const cfg::PlatformPortConfigOverrideFactor& factor) {
  if (auto overridePorts = factor.ports_ref()) {
    if (!portID_.has_value() ||
        std::find(
            overridePorts->begin(),
            overridePorts->end(),
            static_cast<int>(portID_.value())) == overridePorts->end()) {
      return false;
    }
  }
  if (auto overrideProfiles = factor.profiles_ref()) {
    if (std::find(
            overrideProfiles->begin(), overrideProfiles->end(), profileID_) ==
        overrideProfiles->end()) {
      return false;
    }
  }
  if (auto overrideCableLength = factor.cableLengths_ref()) {
    if (!portConfigOverrideFactor_ ||
        !portConfigOverrideFactor_->cableLengths_ref()) {
      return false;
    }
    for (auto cableLength : *portConfigOverrideFactor_->cableLengths_ref()) {
      if (std::find(
              overrideCableLength->begin(),
              overrideCableLength->end(),
              cableLength) == overrideCableLength->end()) {
        return false;
      }
    }
  }
  if (auto overrideMediaInterfaceCode = factor.mediaInterfaceCode_ref()) {
    if (!portConfigOverrideFactor_ ||
        portConfigOverrideFactor_->mediaInterfaceCode_ref() !=
            overrideMediaInterfaceCode) {
      return false;
    }
  }
  if (auto overrideTransceiverManagementInterface =
          factor.transceiverManagementInterface_ref()) {
    if (!portConfigOverrideFactor_ ||
        portConfigOverrideFactor_->transceiverManagementInterface_ref() !=
            overrideTransceiverManagementInterface) {
      return false;
    }
  }
  if (auto overrideChips = factor.chips_ref()) {
    if (!portConfigOverrideFactor_ || !portConfigOverrideFactor_->chips_ref()) {
      return false;
    }
    for (const auto& chip : *portConfigOverrideFactor_->chips_ref()) {
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
  if (factor.get_profileID() != profileID_) {
    return false;
  }
  // if we dont have pimID but the factor does, try to get it from portID
  if (!pimID_.has_value() && factor.pimIDs_ref().has_value() &&
      portID_.has_value()) {
    pimID_ = pm->getPimID(portID_.value());
  }
  if (auto pimIDs = factor.pimIDs_ref()) {
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
  auto mapping =
      apache::thrift::SimpleJSONSerializer::deserialize<cfg::PlatformMapping>(
          jsonPlatformMappingStr);
  platformPorts_ = std::move(*mapping.ports_ref());
  platformSupportedProfiles_ =
      std::move(*mapping.platformSupportedProfiles_ref());
  for (auto chip : *mapping.chips_ref()) {
    chips_[*chip.name_ref()] = chip;
  }
  if (auto portConfigOverrides = mapping.portConfigOverrides_ref()) {
    portConfigOverrides_ = std::move(*portConfigOverrides);
  }
}

cfg::PlatformMapping PlatformMapping::toThrift() const {
  cfg::PlatformMapping newMapping;
  newMapping.ports_ref() = this->platformPorts_;
  newMapping.platformSupportedProfiles_ref() = this->platformSupportedProfiles_;
  for (auto nameChipPair : this->chips_) {
    newMapping.chips_ref()->push_back(nameChipPair.second);
  }
  newMapping.portConfigOverrides_ref() = this->portConfigOverrides_;
  return newMapping;
}

void PlatformMapping::merge(PlatformMapping* mapping) {
  for (auto port : mapping->platformPorts_) {
    platformPorts_.emplace(port.first, std::move(port.second));
    mergePortConfigOverrides(
        port.first, mapping->getPortConfigOverrides(port.first));
  }
  mapping->platformPorts_.clear();

  for (auto incomingProfile : mapping->platformSupportedProfiles_) {
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
    auto currentFactor = currentProfile.factor_ref();
    auto incomingFactor = incomingProfile.factor_ref();
    // Merge factors if profileID and profile config are the same
    if (incomingFactor->profileID_ref() == currentFactor->profileID_ref()) {
      if (incomingProfile.profile_ref() == currentProfile.profile_ref()) {
        // if our currect factor has no pim restriction (supports all pims),
        // skip merging
        if (!currentFactor->pimIDs_ref().has_value()) {
          continue;
        }
        if (auto incomingPims = incomingFactor->pimIDs_ref()) {
          currentFactor->pimIDs_ref()->insert(
              incomingPims->begin(), incomingPims->end());
        }
        // found where to merge factors, we can return here
        return;
      } else if (
          currentFactor->pimIDs_ref().has_value() &&
          incomingFactor->pimIDs_ref().has_value()) {
        // If profileIDs are the same but the profile configs are different,
        // then the pimIDs better be mutually exclusive, otherwise there is no
        // reasonable merge
        std::vector<int> intersection;
        std::set_intersection(
            currentFactor->pimIDs_ref()->begin(),
            currentFactor->pimIDs_ref()->end(),
            incomingFactor->pimIDs_ref()->begin(),
            incomingFactor->pimIDs_ref()->end(),
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
  auto& portName = platformPort.get_mapping().get_name();
  re2::RE2 portNameRe(kFbossPortNameRegex);
  if (!re2::RE2::FullMatch(portName, portNameRe, &pimID)) {
    throw FbossError(
        "Invalid port name: ",
        portName,
        " for port id: ",
        platformPort.get_mapping().get_id());
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
      itPlatformPort->second.mapping_ref()->pins_ref()[0].a_ref()->get_chip();
  return chips_.at(coreName);
}

cfg::PortSpeed PlatformMapping::getPortMaxSpeed(PortID portID) const {
  auto itPlatformPort = platformPorts_.find(portID);
  if (itPlatformPort == platformPorts_.end()) {
    throw FbossError("Unrecoganized port:", portID);
  }

  cfg::PortSpeed maxSpeed{cfg::PortSpeed::DEFAULT};
  for (auto profile : *itPlatformPort->second.supportedProfiles_ref()) {
    if (auto profileConfig = getPortProfileConfig(
            PlatformPortProfileConfigMatcher(profile.first, portID))) {
      if (static_cast<int>(maxSpeed) <
          static_cast<int>(*profileConfig->speed_ref())) {
        maxSpeed = *profileConfig->speed_ref();
      }
    }
  }
  return maxSpeed;
}

std::vector<phy::PinConfig> PlatformMapping::getPortIphyPinConfigs(
    PlatformPortProfileConfigMatcher matcher) const {
  std::optional<phy::DataPlanePhyChip> chip;
  if (auto overrideFactor = matcher.getPortConfigOverrideFactorIf()) {
    if (auto chips = overrideFactor->chips_ref(); chips && !chips->empty()) {
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
    const auto& iphyCfg = *platformPortConfig.pins_ref()->iphy_ref();
    // Check whether there's an override
    for (const auto portConfigOverride : portConfigOverrides_) {
      if (!portConfigOverride.pins_ref().has_value()) {
        // The override is not about Iphy pin configs. Skip
        continue;
      }
      if (matcher.matchOverrideWithFactor(*portConfigOverride.factor_ref())) {
        auto overrideIphy = *portConfigOverride.pins_ref()->iphy_ref();
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
            pinCfg.id_ref() = *iphyCfg.at(i).id_ref();
            // Default to the first entry if we run out
            const auto& override =
                overrideIphy.at(i < overrideIphy.size() ? i : 0);
            if (auto tx = override.tx_ref()) {
              pinCfg.tx_ref() = *tx;
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
    for (const auto portConfigOverride : portConfigOverrides_) {
      if (!portConfigOverride.pins_ref().has_value()) {
        // The override is not about Iphy pin configs. Skip
        continue;
      }
      if (matcher.matchOverrideWithFactor(*portConfigOverride.factor_ref())) {
        return *portConfigOverride.pins_ref()->iphy_ref();
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
  if (auto transceiverPins = platformPortConfig.pins_ref()->transceiver_ref()) {
    return *transceiverPins;
  }
  return std::nullopt;
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
      ? platformPortConfig.pins_ref()->xphySys_ref()
      : platformPortConfig.pins_ref()->xphyLine_ref();
  if (!xphySideOptional.has_value()) {
    return std::vector<phy::PinConfig>();
  }
  const auto& xphySideCfg = xphySideOptional.value();
  // Check whether there's an override
  for (const auto portConfigOverride : portConfigOverrides_) {
    if (!portConfigOverride.pins_ref().has_value()) {
      // The override is not about pin configs. Skip
      continue;
    }
    auto overrideXphySideOptional = side == phy::Side::SYSTEM
        ? portConfigOverride.pins_ref()->xphySys_ref()
        : portConfigOverride.pins_ref()->xphyLine_ref();
    if (!overrideXphySideOptional.has_value()) {
      // The override is not about xphy configs. Skip
      continue;
    }
    if (matcher.matchOverrideWithFactor(*portConfigOverride.factor_ref())) {
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
          *pinCfg.id_ref() = *xphySideCfg.at(i).id_ref();
          // Default to the first entry if we run out
          const auto& override =
              overrideXphySideCfg.at(i < overrideXphySideCfg.size() ? i : 0);
          if (auto tx = override.tx_ref()) {
            pinCfg.tx_ref() = *tx;
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
  newPortPinConfig.xphySys_ref() =
      getPortXphySidePinConfigs(matcher, phy::Side::SYSTEM);
  newPortPinConfig.xphyLine_ref() =
      getPortXphySidePinConfigs(matcher, phy::Side::LINE);
  return newPortPinConfig;
}

const std::optional<phy::PortProfileConfig>
PlatformMapping::getPortProfileConfig(
    PlatformPortProfileConfigMatcher profileMatcher) const {
  for (const auto portConfigOverride : portConfigOverrides_) {
    if (!portConfigOverride.portProfileConfig_ref().has_value()) {
      // The override is not about portProfileConfig. Skip
      continue;
    }
    if (profileMatcher.matchOverrideWithFactor(
            *portConfigOverride.factor_ref())) {
      return *portConfigOverride.portProfileConfig_ref();
    }
  }
  for (auto& supportedProfile : platformSupportedProfiles_) {
    if (profileMatcher.matchProfileWithFactor(
            this, supportedProfile.get_factor())) {
      return supportedProfile.get_profile();
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
    if (auto portList = portConfigOverride.factor_ref()->ports_ref()) {
      if (std::find(portList->begin(), portList->end(), port) !=
          portList->end()) {
        overrides.push_back(portConfigOverride);
      }
    } else if (auto chipList = portConfigOverride.factor_ref()->chips_ref()) {
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
      if (portOverrides.pins_ref() != curOverride.pins_ref() ||
          portOverrides.factor_ref()->profiles_ref() !=
              curOverride.factor_ref()->profiles_ref() ||
          portOverrides.factor_ref()->cableLengths_ref() !=
              curOverride.factor_ref()->cableLengths_ref() ||
          portOverrides.factor_ref()->mediaInterfaceCode_ref() !=
              curOverride.factor_ref()->mediaInterfaceCode_ref() ||
          portOverrides.factor_ref()->chips_ref() !=
              curOverride.factor_ref()->chips_ref()) {
        numMismatch++;
        continue;
      }

      auto portList = portOverrides.factor_ref()->ports_ref();
      auto curPortList = curOverride.factor_ref()->ports_ref();
      if (portList && curPortList) {
        curPortList->push_back(port);
      }
    }
    // if none of the existing override matches, add this override directly
    if (numMismatch == portConfigOverrides_.size()) {
      if (auto portList = portOverrides.factor_ref()->ports_ref()) {
        cfg::PlatformPortConfigOverride newOverride;
        *newOverride.factor_ref() = *portOverrides.factor_ref();
        newOverride.factor_ref()->ports_ref() = {port};
        newOverride.pins_ref().copy_from(portOverrides.pins_ref());
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

const cfg::PlatformPortConfig& PlatformMapping::getPlatformPortConfig(
    PortID id,
    cfg::PortProfileID profileID) const {
  auto itPlatformPort = platformPorts_.find(id);
  if (itPlatformPort == platformPorts_.end()) {
    throw FbossError("No PlatformPortEntry found for port ", id);
  }

  auto& supportedProfiles = *itPlatformPort->second.supportedProfiles_ref();
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

} // namespace fboss
} // namespace facebook
