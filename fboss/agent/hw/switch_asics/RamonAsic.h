// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/switch_asics/BroadcomAsic.h"

namespace facebook::fboss {

class RamonAsic : public BroadcomAsic {
 public:
  RamonAsic(
      std::optional<int64_t> switchId,
      cfg::SwitchInfo switchInfo,
      std::optional<cfg::SdkVersion> sdkVersion = std::nullopt,
      FabricNodeRole fabricNodeRole = FabricNodeRole::SINGLE_STAGE_L1)
      : BroadcomAsic(
            switchId,
            switchInfo,
            sdkVersion,
            {cfg::SwitchType::FABRIC}),
        fabricNodeRole_(fabricNodeRole) {}

  bool isSupported(Feature feature) const override;
  const std::map<cfg::PortType, cfg::PortLoopbackMode>& desiredLoopbackModes()
      const override;
  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_RAMON;
  }
  phy::DataPlanePhyChipType getDataPlanePhyChipType() const override {
    return phy::DataPlanePhyChipType::IPHY;
  }

  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::FOURHUNDREDG;
  }

  std::set<cfg::StreamType> getQueueStreamTypes(
      cfg::PortType portType) const override;
  int getDefaultNumPortQueues(
      cfg::StreamType streamType,
      cfg::PortType portType) const override;
  uint32_t getMaxLabelStackDepth() const override;
  uint64_t getMMUSizeBytes() const override;
  uint32_t getMaxMirrors() const override;
  uint64_t getDefaultReservedBytes(
      cfg::StreamType streamType,
      cfg::PortType portType) const override;
  cfg::MMUScalingFactor getDefaultScalingFactor(
      cfg::StreamType streamType,
      bool cpu) const override;
  int getMaxNumLogicalPorts() const override;
  uint16_t getMirrorTruncateSize() const override;
  uint32_t getMaxWideEcmpSize() const override;
  uint32_t getMaxLagMemberSize() const override;
  uint32_t getSflowShimHeaderSize() const override {
    return 0;
  }
  std::optional<uint32_t> getPortSerdesPreemphasis() const override {
    return std::nullopt;
  }
  uint32_t getPacketBufferUnitSize() const override;
  uint32_t getPacketBufferDescriptorSize() const override;
  uint32_t getMaxVariableWidthEcmpSize() const override;
  uint32_t getMaxEcmpSize() const override;
  uint32_t getNumCores() const override;
  bool scalingFactorBasedDynamicThresholdSupported() const override;
  int getBufferDynThreshFromScalingFactor(
      cfg::MMUScalingFactor scalingFactor) const override;
  uint32_t getStaticQueueLimitBytes() const override;
  uint32_t getNumMemoryBuffers() const override;
  uint32_t getThresholdGranularity() const override {
    return 1024;
  }
  uint32_t getMaxHashSeedLength() const override {
    return 16;
  }
  FabricNodeRole getFabricNodeRole() const override {
    return fabricNodeRole_;
  }
  int getMidPriCpuQueueId() const override {
    throw FbossError("Ramon ASIC does not support cpu queue");
  }
  int getHiPriCpuQueueId() const override {
    throw FbossError("Ramon ASIC does not support cpu queue");
  }

 private:
  FabricNodeRole fabricNodeRole_;
};
} // namespace facebook::fboss
