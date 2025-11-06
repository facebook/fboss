// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

class Agera3PhyAsic : public HwAsic {
 public:
  Agera3PhyAsic(
      std::optional<int64_t> switchId,
      const cfg::SwitchInfo& switchInfo,
      std::optional<cfg::SdkVersion> sdkVersion = std::nullopt)
      : HwAsic(
            switchId,
            switchInfo,
            std::move(sdkVersion),
            {cfg::SwitchType::PHY}) {}
  bool isSupported(Feature feature) const override;
  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_AGERA3;
  }
  std::string getVendor() const override {
    return "broadcom";
  }

  phy::DataPlanePhyChipType getDataPlanePhyChipType() const override {
    return phy::DataPlanePhyChipType::XPHY;
  }

  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::EIGHTHUNDREDG;
  }

  std::set<cfg::StreamType> getQueueStreamTypes(
      cfg::PortType portType) const override;
  int getDefaultNumPortQueues(
      cfg::StreamType streamType,
      cfg::PortType portType) const override;
  uint32_t getMaxLabelStackDepth() const override;
  uint64_t getMMUSizeBytes() const override;
  uint64_t getSramSizeBytes() const override;
  uint32_t getMaxMirrors() const override;
  std::optional<uint64_t> getDefaultReservedBytes(
      cfg::StreamType streamType,
      cfg::PortType portType) const override;
  std::optional<cfg::MMUScalingFactor> getDefaultScalingFactor(
      cfg::StreamType streamType,
      bool cpu) const override;
  int getMaxNumLogicalPorts() const override;
  uint16_t getMirrorTruncateSize() const override;
  uint32_t getMaxWideEcmpSize() const override;
  uint32_t getMaxLagMemberSize() const override;
  uint32_t getSflowShimHeaderSize() const override {
    // TODO Protick check the usage
    return 0;
  }
  std::optional<uint32_t> getPortSerdesPreemphasis() const override {
    return std::nullopt;
  }
  uint32_t getPacketBufferUnitSize() const override;
  uint32_t getPacketBufferDescriptorSize() const override;
  AsicVendor getAsicVendor() const override {
    return HwAsic::AsicVendor::ASIC_VENDOR_BCM;
  }
  uint32_t getMaxVariableWidthEcmpSize() const override;
  uint32_t getMaxEcmpSize() const override;
  uint32_t getNumCores() const override;
  bool scalingFactorBasedDynamicThresholdSupported() const override;
  int getBufferDynThreshFromScalingFactor(
      cfg::MMUScalingFactor scalingFactor) const override;
  uint32_t getStaticQueueLimitBytes() const override;
  uint32_t getNumMemoryBuffers() const override;
  int getMidPriCpuQueueId() const override;
  int getHiPriCpuQueueId() const override;
  std::optional<uint32_t> getMaxArsGroups() const override;
  std::optional<uint32_t> getArsBaseIndex() const override;
};
} // namespace facebook::fboss
