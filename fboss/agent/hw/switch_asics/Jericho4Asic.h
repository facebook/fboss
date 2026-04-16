// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <optional>
#include "fboss/agent/hw/switch_asics/BroadcomAsic.h"

namespace facebook::fboss {

class Jericho4Asic : public BroadcomAsic {
 public:
  Jericho4Asic(
      std::optional<int64_t> switchId,
      const cfg::SwitchInfo& switchInfo,
      std::optional<cfg::SdkVersion> sdkVersion = std::nullopt)
      : BroadcomAsic(
            switchId,
            switchInfo,
            std::move(sdkVersion),
            {cfg::SwitchType::VOQ}) {}
  bool isSupported(Feature) const override;
  const std::map<cfg::PortType, cfg::PortLoopbackMode>& desiredLoopbackModes()
      const override;
  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_JERICHO4;
  }
  AsicMode getAsicMode() const override;
  phy::DataPlanePhyChipType getDataPlanePhyChipType() const override {
    return phy::DataPlanePhyChipType::IPHY;
  }
  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::ONEPOINTSIXT;
  }
  std::set<cfg::StreamType> getQueueStreamTypes(
      cfg::PortType portType) const override;
  int getDefaultNumPortQueues(
      cfg::StreamType streamType,
      cfg::PortType portType) const override;
  uint32_t getMaxLabelStackDepth() const override {
    // TODO: update these values for J4
    return 9;
  }
  uint64_t getMMUSizeBytes() const override {
    // TODO: update these values for J4
    return uint64_t(12) * 1024 * 1024 * 1024;
  }

  uint64_t getSramSizeBytes() const override {
    // TODO: update these values for J4
    return 128 * 1024 * 1024;
  }

  uint32_t getMaxSwitchId() const override {
    // TODO: update these values for J4
    return 4064;
  }
  uint32_t getMMUCellSize() const {
    // TODO: update these values for J4
    return 254;
  }
  uint32_t getMaxMirrors() const override {
    // TODO: update these values for J4
    return 4;
  }
  std::optional<uint64_t> getDefaultReservedBytes(
      cfg::StreamType streamType,
      cfg::PortType portType) const override;
  std::optional<cfg::MMUScalingFactor> getDefaultScalingFactor(
      cfg::StreamType /*streamType*/,
      bool /*cpu*/) const override {
    // TODO: update these values for J4
    return cfg::MMUScalingFactor::TWO;
  }
  int getMaxNumLogicalPorts() const override {
    // TODO: update these values for J4
    return 160;
  }
  uint16_t getMirrorTruncateSize() const override {
    // TODO: update these values for J4
    return 128;
  }
  uint32_t getMaxWideEcmpSize() const override {
    // TODO: update these values for J4
    return 2048;
  }
  uint32_t getMaxLagMemberSize() const override {
    // TODO: update these values for J4
    return 64;
  }
  uint32_t getPacketBufferUnitSize() const override {
    // TODO: update these values for J4
    return 254;
  }
  uint32_t getPacketBufferDescriptorSize() const override {
    // TODO: update these values for J4
    return 40;
  }
  uint32_t getMaxVariableWidthEcmpSize() const override {
    // TODO: update these values for J4
    return 2048;
  }
  uint32_t getMaxEcmpSize() const override {
    // TODO: update these values for J4
    return 2048;
  }
  std::optional<uint32_t> getMaxEcmpGroups() const override;
  std::optional<uint32_t> getMaxEcmpMembers() const override {
    // TODO: update these values for J4
    return 32768;
  }
  uint32_t getSflowShimHeaderSize() const override {
    // TODO: update these values for J4
    return 104;
  }
  std::optional<uint32_t> getPortSerdesPreemphasis() const override {
    return std::nullopt;
  }
  uint32_t getStaticQueueLimitBytes() const override {
    // TODO: update these values for J4
    return static_cast<uint32_t>(getMMUSizeBytes() / 2);
  }
  uint32_t getNumCores() const override {
    // Jericho4 cModel bringup is using Jericho4L (99410) which only has 4 cores
    return 4;
  }
  bool scalingFactorBasedDynamicThresholdSupported() const override {
    return true;
  }
  cfg::Range64 getReservedEncapIndexRange() const override;
  HwAsic::RecyclePortInfo getRecyclePortInfo(
      InterfaceNodeRole intfRole) const override;
  uint32_t getNumMemoryBuffers() const override {
    // TODO: update these values for J4
    return 3;
  }
  int getBufferDynThreshFromScalingFactor(
      cfg::MMUScalingFactor scalingFactor) const override {
    if (scalingFactor == cfg::MMUScalingFactor::ONE_32768TH) {
      // TODO: update these values for J4
      return -15;
    }
    return HwAsic::getBufferDynThreshFromScalingFactor(scalingFactor);
  }
  uint32_t getThresholdGranularity() const override {
    // TODO: update these values for J4
    return 1024;
  }
  uint32_t getMaxHashSeedLength() const override {
    // TODO: update these values for J4
    return 16;
  }
  std::optional<uint32_t> computePortGroupSkew(
      const std::map<PortID, uint32_t>& portId2cableLen) const override;
  std::vector<std::pair<int, int>> getPortGroups() const;
  int getMidPriCpuQueueId() const override;
  int getHiPriCpuQueueId() const override;
};

} // namespace facebook::fboss
