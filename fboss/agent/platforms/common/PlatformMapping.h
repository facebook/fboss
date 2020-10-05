/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook {
namespace fboss {

class PlatformPortConfigOverrideFactorMatcher {
 public:
  explicit PlatformPortConfigOverrideFactorMatcher(
      facebook::fboss::PortID portID,
      facebook::fboss::cfg::PortProfileID profileID,
      std::optional<double> cableLength)
      : portID_(portID), profileID_(profileID), cableLength_(cableLength) {}

  explicit PlatformPortConfigOverrideFactorMatcher(
      facebook::fboss::cfg::PortProfileID profileID,
      ExtendedSpecComplianceCode transceiverSpecComplianceCode)
      : profileID_(profileID),
        transceiverSpecComplianceCode_(transceiverSpecComplianceCode) {}

  bool matchWithFactor(
      const facebook::fboss::cfg::PlatformPortConfigOverrideFactor& factor) {
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
      if (!profileID_.has_value() ||
          std::find(
              overrideProfiles->begin(),
              overrideProfiles->end(),
              profileID_.value()) == overrideProfiles->end()) {
        return false;
      }
    }
    if (auto overrideCableLength = factor.cableLengths_ref()) {
      if (!cableLength_.has_value() ||
          std::find(
              overrideCableLength->begin(),
              overrideCableLength->end(),
              cableLength_.value()) == overrideCableLength->end()) {
        return false;
      }
    }
    if (auto overrideTransceiverSpecComplianceCode =
            factor.transceiverSpecComplianceCode_ref()) {
      if (!transceiverSpecComplianceCode_.has_value() ||
          transceiverSpecComplianceCode_.value() !=
              overrideTransceiverSpecComplianceCode) {
        return false;
      }
    }
    return true;
  }

 private:
  std::optional<facebook::fboss::PortID> portID_;
  std::optional<facebook::fboss::cfg::PortProfileID> profileID_;
  std::optional<double> cableLength_;
  std::optional<ExtendedSpecComplianceCode> transceiverSpecComplianceCode_;
};

class PlatformMapping {
 public:
  PlatformMapping() {}
  explicit PlatformMapping(const std::string& jsonPlatformMappingStr);
  virtual ~PlatformMapping() = default;

  cfg::PlatformMapping toThrift() const;

  const std::map<int32_t, cfg::PlatformPortEntry>& getPlatformPorts() const {
    return platformPorts_;
  }

  const std::map<cfg::PortProfileID, phy::PortProfileConfig>&
  getSupportedProfiles() const {
    return supportedProfiles_;
  }

  const std::optional<phy::PortProfileConfig> getPortProfileConfig(
      cfg::PortProfileID PortProfileID,
      std::optional<ExtendedSpecComplianceCode> transceiverSpecComplianceCode)
      const;

  std::vector<phy::PinConfig> getPortIphyPinConfigs(
      PortID id,
      cfg::PortProfileID profileID,
      std::optional<double> cableLength = std::nullopt) const;

  const std::map<std::string, phy::DataPlanePhyChip>& getChips() const {
    return chips_;
  }

  void setPlatformPort(int32_t portID, cfg::PlatformPortEntry port) {
    platformPorts_.emplace(portID, port);
  }

  void setChip(const std::string& chipName, phy::DataPlanePhyChip chip) {
    chips_.emplace(chipName, chip);
  }

  void setSupportedProfile(
      cfg::PortProfileID profileID,
      phy::PortProfileConfig profile) {
    supportedProfiles_.emplace(profileID, profile);
  }

  void merge(PlatformMapping* mapping);

  cfg::PortProfileID getPortMaxSpeedProfile(PortID portID) const;
  cfg::PortSpeed getPortMaxSpeed(PortID portID) const;

  const std::vector<cfg::PlatformPortConfigOverride>& getPortConfigOverrides()
      const {
    return portConfigOverrides_;
  }
  std::vector<cfg::PlatformPortConfigOverride> getPortConfigOverrides(
      int32_t port) const;
  void mergePortConfigOverrides(
      int32_t port,
      std::vector<cfg::PlatformPortConfigOverride> overrides);

 protected:
  std::map<int32_t, cfg::PlatformPortEntry> platformPorts_;
  std::map<cfg::PortProfileID, phy::PortProfileConfig> supportedProfiles_;
  std::map<std::string, phy::DataPlanePhyChip> chips_;
  std::vector<cfg::PlatformPortConfigOverride> portConfigOverrides_;

 private:
  // Forbidden copy constructor and assignment operator
  PlatformMapping(PlatformMapping const&) = delete;
  PlatformMapping& operator=(PlatformMapping const&) = delete;
};
} // namespace fboss
} // namespace facebook
