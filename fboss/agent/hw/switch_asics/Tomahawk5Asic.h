// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/BroadcomXgsAsic.h"

namespace facebook::fboss {

class Tomahawk5Asic : public BroadcomXgsAsic {
 public:
  using BroadcomXgsAsic::BroadcomXgsAsic;
  bool isSupported(Feature) const override;
  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_TOMAHAWK5;
  }
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
    return cfg::PortSpeed::FOURHUNDREDG;
  }
  int getDefaultNumPortQueues(cfg::StreamType streamType, bool cpu)
      const override;
  uint32_t getMaxLabelStackDepth() const override {
    return 9;
  }
  uint64_t getMMUSizeBytes() const override {
    return 2 * 341080 * 254;
  }
  uint32_t getMMUCellSize() const {
    return 254;
  }
  uint64_t getDefaultReservedBytes(cfg::StreamType /*streamType*/, bool cpu)
      const override {
    /* TODO: Mimicking TH3 size here*/
    return cpu ? 1778 : 0;
  }
  cfg::MMUScalingFactor getDefaultScalingFactor(
      cfg::StreamType /*streamType*/,
      bool /*cpu*/) const override {
    /* TODO: Mimicking TH3 size here*/
    return cfg::MMUScalingFactor::TWO;
  }

  int getMaxNumLogicalPorts() const override {
    return 341;
  }
  uint16_t getMirrorTruncateSize() const override {
    // TODO: update numbers if necessary
    return 254;
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
