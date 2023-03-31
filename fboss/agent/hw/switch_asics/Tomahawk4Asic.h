// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/BroadcomXgsAsic.h"

DECLARE_int32(teFlow_gid);

namespace facebook::fboss {

class Tomahawk4Asic : public BroadcomXgsAsic {
 public:
  using BroadcomXgsAsic::BroadcomXgsAsic;
  bool isSupported(Feature) const override;
  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_TOMAHAWK4;
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
    // one VC label and 8 tunnel labels, same as tomahawk3
    return 9;
  }
  uint64_t getMMUSizeBytes() const override {
    return 2 * 234606 * 254;
  }
  uint32_t getMMUCellSize() const {
    return 254;
  }
  uint64_t getDefaultReservedBytes(cfg::StreamType /*streamType*/, bool cpu)
      const override {
    /* TODO: Mimicking TH3 size here, figure out the defaults for TH4*/
    return cpu ? 1778 : 0;
  }
  cfg::MMUScalingFactor getDefaultScalingFactor(
      cfg::StreamType /*streamType*/,
      bool /*cpu*/) const override {
    /* TODO: Mimicking TH3 size here, figure out the defaults for TH4*/
    return cfg::MMUScalingFactor::TWO;
  }

  int getNumLanesPerPhysicalPort() const override;

  int getDefaultACLGroupID() const override;

  int getDefaultTeFlowGroupID() const override;

  int getStationID(int intfId) const override;

  int getMaxNumLogicalPorts() const override {
    return 272;
  }
  uint16_t getMirrorTruncateSize() const override {
    return 254;
  }

  uint32_t getMaxWideEcmpSize() const override {
    return 128;
  }
  uint32_t getMaxLagMemberSize() const override {
    return 64;
  }
  uint32_t getPacketBufferUnitSize() const override {
    return 254;
  }
  uint32_t getPacketBufferDescriptorSize() const override {
    return 48;
  }
  uint32_t getMaxVariableWidthEcmpSize() const override {
    return 512;
  }
  uint32_t getMaxEcmpSize() const override {
    return 4096;
  }
  uint32_t getStaticQueueLimitBytes() const override {
    // Per ITM buffers limits the queue size
    return getMMUSizeBytes() / 2;
  }
  uint32_t getNumMemoryBuffers() const override {
    return 2;
  }
};

} // namespace facebook::fboss
