// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/BroadcomAsic.h"

namespace facebook::fboss {

class Trident2Asic : public BroadcomAsic {
 public:
  using BroadcomAsic::BroadcomAsic;
  bool isSupported(Feature) const override;
  AsicType getAsicType() const override {
    return AsicType::ASIC_TYPE_TRIDENT2;
  }
  phy::DataPlanePhyChipType getDataPlanePhyChipType() const override {
    return phy::DataPlanePhyChipType::IPHY;
  }
  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::FORTYG;
  }
  std::set<cfg::StreamType> getQueueStreamTypes(bool cpu) const override {
    if (cpu) {
      return {cfg::StreamType::MULTICAST};
    } else {
      return {cfg::StreamType::UNICAST};
    }
  }
  int getDefaultNumPortQueues(cfg::StreamType streamType, bool cpu)
      const override;
  uint32_t getMaxLabelStackDepth() const override {
    return 2;
  }
  uint64_t getMMUSizeBytes() const override {
    return 16 * 1024 * 1024;
  }
  uint32_t getMMUCellSize() const {
    return 208;
  }
  cfg::PortLoopbackMode desiredLoopbackMode() const override {
    // Changing loopback mode to MAC on a 40G port on trident2 changes
    // the speed to 10G unexpectedly.
    //
    // Broadcom case: CS8832244
    //
    return cfg::PortLoopbackMode::PHY;
  }
  uint64_t getDefaultReservedBytes(cfg::StreamType /*streamType*/, bool cpu)
      const override {
    return cpu ? 1664 : 0;
  }
  cfg::MMUScalingFactor getDefaultScalingFactor(
      cfg::StreamType /*streamType*/,
      bool /*cpu*/) const override {
    return cfg::MMUScalingFactor::TWO;
  }
  int getMaxNumLogicalPorts() const override {
    return 65;
  }
  uint16_t getMirrorTruncateSize() const override {
    return 0;
  }
  uint32_t getMaxWideEcmpSize() const override {
    return 128;
  }
  uint32_t getMaxLagMemberSize() const override {
    return 256;
  }
  uint32_t getPacketBufferUnitSize() const override {
    return 208;
  }
  uint32_t getPacketBufferDescriptorSize() const override {
    return 64;
  }
  uint32_t getMaxVariableWidthEcmpSize() const override {
    return 1024;
  }
  uint32_t getMaxEcmpSize() const override {
    return 1024;
  }
  uint32_t getStaticQueueLimitBytes() const override {
    return getMMUSizeBytes();
  }
};

} // namespace facebook::fboss
