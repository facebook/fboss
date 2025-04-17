// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/BroadcomXgsAsic.h"

namespace facebook::fboss {

class Tomahawk6Asic : public BroadcomXgsAsic {
 public:
  using BroadcomXgsAsic::BroadcomXgsAsic;
  bool isSupported(Feature) const override;
  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_TOMAHAWK6;
  }
  const std::map<cfg::PortType, cfg::PortLoopbackMode>& desiredLoopbackModes()
      const override;
  phy::DataPlanePhyChipType getDataPlanePhyChipType() const override {
    return phy::DataPlanePhyChipType::IPHY;
  }
  AsicMode getAsicMode() const override {
    static const AsicMode asicMode = std::getenv("BCM_SIM_PATH")
        ? AsicMode::ASIC_MODE_SIM
        : AsicMode::ASIC_MODE_HW;
    return asicMode;
  }
  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::EIGHTHUNDREDG;
  }
  int getDefaultNumPortQueues(
      cfg::StreamType streamType,
      cfg::PortType portType) const override;
  uint32_t getMaxLabelStackDepth() const override {
    return 9;
  }
  uint64_t getMMUSizeBytes() const override {
    return 2 * 341080 * 254;
  }
  uint64_t getSramSizeBytes() const override {
    // No HBM!
    return getMMUSizeBytes();
  }
  uint32_t getMMUCellSize() const {
    return 254;
  }
  std::optional<uint64_t> getDefaultReservedBytes(
      cfg::StreamType /*streamType*/,
      cfg::PortType portType) const override {
    /* TODO: Mimicking TH3 size here*/
    return portType == cfg::PortType::CPU_PORT ? 1778 : 0;
  }
  std::optional<cfg::MMUScalingFactor> getDefaultScalingFactor(
      cfg::StreamType /*streamType*/,
      bool /*cpu*/) const override {
    /* TODO: Mimicking TH3 size here*/
    return cfg::MMUScalingFactor::TWO;
  }

  int getMaxNumLogicalPorts() const override {
    return 341;
  }
  uint16_t getMirrorTruncateSize() const override {
    return 204;
  }

  uint32_t getMaxWideEcmpSize() const override {
    // TODO: update numbers if necessary
    return 128;
  }
  uint32_t getMaxLagMemberSize() const override {
    // TODO: update numbers if necessary
    return 64;
  }
  uint32_t getPacketBufferUnitSize() const override {
    // TODO: update numbers if necessary
    return 254;
  }
  uint32_t getPacketBufferDescriptorSize() const override {
    // TODO: update numbers if necessary
    return 48;
  }
  uint32_t getMaxVariableWidthEcmpSize() const override {
    // TODO: update numbers if necessary
    return 512;
  }
  uint32_t getMaxEcmpSize() const override {
    // TODO: update numbers if necessary
    return 4096;
  }
  std::optional<uint32_t> getMaxEcmpGroups() const override {
    return 4096;
  }
  std::optional<uint32_t> getMaxEcmpMembers() const override {
    // CS00012330051
    return 32000;
  }
  std::optional<uint32_t> getMaxDlbEcmpGroups() const override {
    // TODO: old TH4 number, update if necessary
    return 128;
  }
  uint32_t getStaticQueueLimitBytes() const override {
    // TODO: update numbers if necessary
    return getMMUSizeBytes() / 2;
  }
  uint32_t getNumMemoryBuffers() const override {
    // TODO: update numbers if necessary
    return 2;
  }
};

} // namespace facebook::fboss
