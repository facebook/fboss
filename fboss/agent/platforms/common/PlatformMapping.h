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

class PlatformMapping;
class PlatformPortProfileConfigMatcher {
 public:
  PlatformPortProfileConfigMatcher(
      cfg::PortProfileID profileID,
      std::optional<PimID> pimID)
      : profileID_(profileID), pimID_(pimID) {}

  PlatformPortProfileConfigMatcher(cfg::PortProfileID profileID, PortID portID)
      : profileID_(profileID), portID_(portID) {}

  PlatformPortProfileConfigMatcher(
      cfg::PortProfileID profileID,
      PortID portID,
      ExtendedSpecComplianceCode transceiverSpecComplianceCode)
      : profileID_(profileID),
        portID_(portID),
        transceiverSpecComplianceCode_(transceiverSpecComplianceCode) {}

  PlatformPortProfileConfigMatcher(
      cfg::PortProfileID profileID,
      PortID portID,
      std::optional<double> cableLength)
      : profileID_(profileID), portID_(portID), cableLength_(cableLength) {}

  bool matchOverrideWithFactor(
      const cfg::PlatformPortConfigOverrideFactor& factor);

  bool matchProfileWithFactor(
      const PlatformMapping* pm,
      const cfg::PlatformPortConfigFactor& factor);

  std::string toString() const;

  cfg::PortProfileID profileID_;
  std::optional<PimID> pimID_;
  std::optional<PortID> portID_;
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

  const std::optional<phy::PortProfileConfig> getPortProfileConfig(
      PlatformPortProfileConfigMatcher matcher) const;

  std::vector<phy::PinConfig> getPortIphyPinConfigs(
      PortID id,
      cfg::PortProfileID profileID,
      std::optional<double> cableLength = std::nullopt) const;

  const std::map<std::string, phy::DataPlanePhyChip>& getChips() const {
    return chips_;
  }

  int getPimID(PortID portID) const;

  int getPimID(const cfg::PlatformPortEntry& platformPort) const;

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

  void mergePlatformSupportedProfile(
      cfg::PlatformPortProfileConfigEntry supportedProfile);

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
  std::vector<cfg::PlatformPortProfileConfigEntry> platformSupportedProfiles_;
  std::map<std::string, phy::DataPlanePhyChip> chips_;
  std::vector<cfg::PlatformPortConfigOverride> portConfigOverrides_;

 private:
  // Forbidden copy constructor and assignment operator
  PlatformMapping(PlatformMapping const&) = delete;
  PlatformMapping& operator=(PlatformMapping const&) = delete;
};
} // namespace fboss
} // namespace facebook
