// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/switch_asics/BroadcomAsic.h"

namespace facebook::fboss {

class Jericho2Asic : public BroadcomAsic {
 public:
  Jericho2Asic(
      std::optional<int64_t> switchId,
      cfg::SwitchInfo switchInfo,
      std::optional<cfg::SdkVersion> sdkVersion = std::nullopt)
      : BroadcomAsic(switchId, switchInfo, sdkVersion, {cfg::SwitchType::VOQ}) {
  }
  bool isSupported(Feature) const override;
  const std::map<cfg::PortType, cfg::PortLoopbackMode>& desiredLoopbackModes()
      const override;
  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_JERICHO2;
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
  uint32_t getMaxLabelStackDepth() const override {
    return 9;
  }
  uint64_t getMMUSizeBytes() const override {
    // DRAM utilization is 75%, ie. only 75% of the total
    // OBM+HBM is available to user of the total ~8G.
    return uint64_t(6) * 1024 * 1024 * 1024;
  }

  uint64_t getSramSizeBytes() const override {
    // 64MB
    return 64 * 1024 * 1024;
  }

  uint32_t getMMUCellSize() const {
    return 254;
  }
  uint32_t getMaxMirrors() const override {
    return 4;
  }
  std::optional<uint64_t> getDefaultReservedBytes(
      cfg::StreamType streamType,
      cfg::PortType portType) const override;
  std::optional<cfg::MMUScalingFactor> getDefaultScalingFactor(
      cfg::StreamType /*streamType*/,
      bool /*cpu*/) const override {
    return cfg::MMUScalingFactor::TWO;
  }
  int getMaxNumLogicalPorts() const override {
    return 160;
  }
  uint16_t getMirrorTruncateSize() const override {
    return 128;
  }
  uint32_t getMaxWideEcmpSize() const override {
    return 512;
  }
  uint32_t getMaxLagMemberSize() const override {
    return 64;
  }
  uint32_t getPacketBufferUnitSize() const override {
    return 4096;
  }
  uint32_t getPacketBufferDescriptorSize() const override {
    return 30;
  }
  uint32_t getMaxVariableWidthEcmpSize() const override {
    return 512;
  }
  uint32_t getMaxEcmpSize() const override {
    return 4096;
  }
  uint32_t getSflowShimHeaderSize() const override {
    return 0;
  }
  std::optional<uint32_t> getPortSerdesPreemphasis() const override {
    return std::nullopt;
  }
  uint32_t getStaticQueueLimitBytes() const override {
    // Per ITM buffers limits the queue size
    return getMMUSizeBytes() / 2;
  }
  uint32_t getNumCores() const override {
    return 2;
  }
  bool scalingFactorBasedDynamicThresholdSupported() const override {
    return true;
  }
  cfg::Range64 getReservedEncapIndexRange() const override;
  HwAsic::RecyclePortInfo getRecyclePortInfo(
      InterfaceNodeRole intfRole) const override;
  uint32_t getNumMemoryBuffers() const override {
    return 2;
  }
  int getBufferDynThreshFromScalingFactor(
      cfg::MMUScalingFactor scalingFactor) const override {
    switch (scalingFactor) {
      case cfg::MMUScalingFactor::ONE_32768TH:
        return -15;
      default:
        return BroadcomAsic::getBufferDynThreshFromScalingFactor(scalingFactor);
    }
  }
  uint32_t getThresholdGranularity() const override {
    return 1024;
  }
  uint32_t getMaxHashSeedLength() const override {
    return 16;
  }
  int getMidPriCpuQueueId() const override;
  int getHiPriCpuQueueId() const override;
};

} // namespace facebook::fboss
