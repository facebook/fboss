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
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

DECLARE_string(platform_mapping_override_path);

namespace facebook {
namespace fboss {

cfg::PlatformPortConfigOverrideFactor buildPlatformPortConfigOverrideFactor(
    const TransceiverInfo& transceiverInfo);

class PlatformMapping;
class PlatformPortProfileConfigMatcher {
 public:
  PlatformPortProfileConfigMatcher(
      cfg::PortProfileID profileID,
      std::optional<PimID> pimID)
      : profileID_(profileID), pimID_(pimID) {}

  explicit PlatformPortProfileConfigMatcher(
      cfg::PortProfileID profileID,
      std::optional<PortID> portID = std::nullopt,
      std::optional<cfg::PlatformPortConfigOverrideFactor>
          portConfigOverrideFactor = std::nullopt)
      : profileID_(profileID),
        portID_(portID),
        portConfigOverrideFactor_(portConfigOverrideFactor) {}

  std::optional<PortID> getPortIDIf() const {
    return portID_;
  }

  std::optional<cfg::PlatformPortConfigOverrideFactor>
  getPortConfigOverrideFactorIf() const {
    return portConfigOverrideFactor_;
  }

  cfg::PortProfileID getProfileID() const {
    return profileID_;
  }

  bool matchOverrideWithFactor(
      const cfg::PlatformPortConfigOverrideFactor& factor);

  bool matchProfileWithFactor(
      const PlatformMapping* pm,
      const cfg::PlatformPortConfigFactor& factor);

  std::string toString() const;

 private:
  const cfg::PortProfileID profileID_;
  std::optional<PimID> pimID_;
  std::optional<PortID> portID_;
  const std::optional<cfg::PlatformPortConfigOverrideFactor>
      portConfigOverrideFactor_;
};

class PlatformMapping {
 public:
  PlatformMapping() {}
  explicit PlatformMapping(const std::string& jsonPlatformMappingStr);
  explicit PlatformMapping(const cfg::PlatformMapping& mapping);
  virtual ~PlatformMapping() = default;

  cfg::PlatformMapping toThrift() const;

  const std::map<int32_t, cfg::PlatformPortEntry>& getPlatformPorts() const {
    return platformPorts_;
  }

  const cfg::PlatformPortEntry& getPlatformPort(int32_t portId) const;

  const std::optional<phy::PortProfileConfig> getPortProfileConfig(
      PlatformPortProfileConfigMatcher matcher) const;

  std::vector<phy::PinConfig> getPortXphySidePinConfigs(
      PlatformPortProfileConfigMatcher matcher,
      phy::Side side) const;

  std::vector<phy::PinConfig> getPortIphyPinConfigs(
      PlatformPortProfileConfigMatcher matcher) const;

  phy::PortPinConfig getPortXphyPinConfig(
      PlatformPortProfileConfigMatcher matcher) const;

  std::optional<std::vector<phy::PinConfig>> getPortTransceiverPinConfigs(
      PlatformPortProfileConfigMatcher matcher) const;

  std::set<uint8_t> getTransceiverHostLanes(
      PlatformPortProfileConfigMatcher matcher) const;

  const std::map<std::string, phy::DataPlanePhyChip>& getChips() const {
    return chips_;
  }

  int getPimID(PortID portID) const;

  int getPimID(const cfg::PlatformPortEntry& platformPort) const;

  const phy::DataPlanePhyChip& getPortIphyChip(PortID port) const;

  void setPlatformPort(int32_t portID, cfg::PlatformPortEntry port) {
    platformPorts_.emplace(portID, port);
  }

  void setChip(const std::string& chipName, phy::DataPlanePhyChip chip) {
    chips_.emplace(chipName, chip);
  }

  void mergePlatformSupportedProfile(
      cfg::PlatformPortProfileConfigEntry supportedProfile);

  void merge(PlatformMapping* mapping);

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

  // Converts port name -> logical portID
  const PortID getPortID(const std::string& portName) const;

  /*
   * Some platforms need customize their raw override factor generated from
   * Transceiver or Chip to match their PlatformMapping PortConfigOverrides
   */
  virtual void customizePlatformPortConfigOverrideFactor(
      std::optional<cfg::PlatformPortConfigOverrideFactor>& /* factor */)
      const {}

  std::map<phy::DataPlanePhyChip, std::vector<phy::PinConfig>>
  getCorePinMapping(const std::vector<cfg::Port>& ports) const;

 protected:
  std::map<int32_t, cfg::PlatformPortEntry> platformPorts_;
  std::vector<cfg::PlatformPortProfileConfigEntry> platformSupportedProfiles_;
  std::map<std::string, phy::DataPlanePhyChip> chips_;
  std::vector<cfg::PlatformPortConfigOverride> portConfigOverrides_;

  const cfg::PlatformPortConfig& getPlatformPortConfig(
      PortID id,
      cfg::PortProfileID profileID) const;

 private:
  void init(const cfg::PlatformMapping& mapping);
  // Forbidden copy constructor and assignment operator
  PlatformMapping(PlatformMapping const&) = delete;
  PlatformMapping& operator=(PlatformMapping const&) = delete;
};
} // namespace fboss
} // namespace facebook
