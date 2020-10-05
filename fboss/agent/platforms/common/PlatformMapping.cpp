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

#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/FbossError.h"

namespace facebook {
namespace fboss {
PlatformMapping::PlatformMapping(const std::string& jsonPlatformMappingStr) {
  auto mapping =
      apache::thrift::SimpleJSONSerializer::deserialize<cfg::PlatformMapping>(
          jsonPlatformMappingStr);
  platformPorts_ = std::move(*mapping.ports_ref());
  supportedProfiles_ = std::move(*mapping.supportedProfiles_ref());
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
  newMapping.supportedProfiles_ref() = this->supportedProfiles_;
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

  for (auto profile : mapping->supportedProfiles_) {
    supportedProfiles_.emplace(profile.first, std::move(profile.second));
  }
  mapping->supportedProfiles_.clear();

  for (auto chip : mapping->chips_) {
    chips_.emplace(chip.first, std::move(chip.second));
  }
  mapping->chips_.clear();
}

cfg::PortProfileID PlatformMapping::getPortMaxSpeedProfile(
    PortID portID) const {
  auto itPlatformPort = platformPorts_.find(portID);
  if (itPlatformPort == platformPorts_.end()) {
    throw FbossError("Unrecoganized port:", portID);
  }

  cfg::PortProfileID maxProfile{cfg::PortProfileID::PROFILE_DEFAULT};
  cfg::PortSpeed maxSpeed{cfg::PortSpeed::DEFAULT};
  for (auto profile : *itPlatformPort->second.supportedProfiles_ref()) {
    if (auto itProfileCfg = supportedProfiles_.find(profile.first);
        itProfileCfg != supportedProfiles_.end() &&
        static_cast<int>(maxSpeed) <
            static_cast<int>(*itProfileCfg->second.speed_ref())) {
      maxSpeed = *itProfileCfg->second.speed_ref();
      maxProfile = itProfileCfg->first;
    }
  }
  return maxProfile;
}

cfg::PortSpeed PlatformMapping::getPortMaxSpeed(PortID portID) const {
  auto maxProfile = getPortMaxSpeedProfile(portID);
  if (auto itProfileCfg = supportedProfiles_.find(maxProfile);
      itProfileCfg != supportedProfiles_.end()) {
    return *itProfileCfg->second.speed_ref();
  }
  return cfg::PortSpeed::DEFAULT;
}

std::vector<phy::PinConfig> PlatformMapping::getPortIphyPinConfigs(
    PortID id,
    cfg::PortProfileID profileID,
    std::optional<double> cableLength) const {
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

  const auto& iphyCfg = *platformPortConfig->second.pins_ref()->iphy_ref();
  // Check whether there's an override
  for (const auto portConfigOverride : portConfigOverrides_) {
    if (!portConfigOverride.pins_ref().has_value()) {
      // The override is not about Iphy pin configs. Skip
      continue;
    }
    auto platformPortConfigOverrideFactorMatcher =
        PlatformPortConfigOverrideFactorMatcher(id, profileID, cableLength);
    if (platformPortConfigOverrideFactorMatcher.matchWithFactor(
            *portConfigOverride.factor_ref())) {
      auto overrideIphy = *portConfigOverride.pins_ref()->iphy_ref();
      if (!overrideIphy.empty()) {
        // make sure the override iphy config size == iphyCfg or override
        // size == 1, in which case we use the same override for all lanes
        if (overrideIphy.size() != iphyCfg.size() && overrideIphy.size() != 1) {
          throw FbossError(
              "Port ",
              id,
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
          *pinCfg.id_ref() = *iphyCfg.at(i).id_ref();
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
}

const std::optional<phy::PortProfileConfig>
PlatformMapping::getPortProfileConfig(
    cfg::PortProfileID profileID,
    std::optional<ExtendedSpecComplianceCode> transceiverSpecComplianceCode)
    const {
  // For now, the PortProfileConfig override will only happen when we have
  // a valid ExtendedSpecComplianceCode. Thus skip looping through the
  // overrides if we don't get a valid value. If new use cases were to come
  // up later, we can extend the logic and the matcher.
  if (transceiverSpecComplianceCode.has_value()) {
    for (const auto portConfigOverride : portConfigOverrides_) {
      if (!portConfigOverride.portProfileConfig_ref().has_value()) {
        // The override is not about portProfileConfig. Skip
        continue;
      }
      auto platformPortConfigOverrideFactorMatcher =
          PlatformPortConfigOverrideFactorMatcher(
              profileID, transceiverSpecComplianceCode.value());
      if (platformPortConfigOverrideFactorMatcher.matchWithFactor(
              *portConfigOverride.factor_ref())) {
        if (auto overridePortProfileConfig =
                portConfigOverride.portProfileConfig_ref()) {
          return *overridePortProfileConfig;
        }
      }
    }
  }
  const auto& supportedProfiles = getSupportedProfiles();
  auto itProfileConfig = supportedProfiles.find(profileID);
  if (itProfileConfig != supportedProfiles.end()) {
    return itProfileConfig->second;
  }
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
    } else {
      // If factor.ports is empty, which means such override apply for all ports
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
          portOverrides.factor_ref()->transceiverSpecComplianceCode_ref() !=
              curOverride.factor_ref()->transceiverSpecComplianceCode_ref()) {
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

} // namespace fboss
} // namespace facebook
