// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/BroadcomXgsAsic.h"

namespace facebook::fboss {

class TomahawkAsic : public BroadcomXgsAsic {
 public:
  using BroadcomXgsAsic::BroadcomXgsAsic;
  bool isSupported(Feature) const override;
  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_TOMAHAWK;
  }
  phy::DataPlanePhyChipType getDataPlanePhyChipType() const override {
    return phy::DataPlanePhyChipType::IPHY;
  }
  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::HUNDREDG;
  }
  int getDefaultNumPortQueues(
      cfg::StreamType streamType,
      cfg::PortType portType) const override;
  uint32_t getMaxLabelStackDepth() const override {
    return 3;
  }
  uint64_t getMMUSizeBytes() const override {
    return 16 * 1024 * 1024;
  }
  uint64_t getSramSizeBytes() const override {
    // No HBM!
    return getMMUSizeBytes();
  }
  std::optional<uint64_t> getDefaultReservedBytes(
      cfg::StreamType /*streamType*/,
      cfg::PortType portType) const override {
    return portType == cfg::PortType::CPU_PORT ? 1664 : 0;
  }
  uint32_t getMMUCellSize() const {
    return 208;
  }
  std::optional<cfg::MMUScalingFactor> getDefaultScalingFactor(
      cfg::StreamType /*streamType*/,
      bool /*cpu*/) const override {
    return cfg::MMUScalingFactor::TWO;
  }
  bool mmuQgroupsEnabled() const override {
    return true;
  }
  int getMaxNumLogicalPorts() const override {
    return 134;
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
    return 128;
  }
  uint32_t getMaxEcmpSize() const override {
    return 128;
  }
  std::optional<uint32_t> getMaxEcmpGroups() const override {
    // 56960-DS113: With Config change(l3_max_ecmp_mode = 1): 1024
    // CS00012341838
    return 895;
  }
  std::optional<uint32_t> getMaxEcmpMembers() const override {
    // 56960-DS113
    return 16000;
  }
  uint32_t getStaticQueueLimitBytes() const override {
    return getMMUSizeBytes();
  }
  uint32_t getNumMemoryBuffers() const override {
    return 4;
  }
  std::optional<uint32_t> getMaxAclTables() const override {
    return 10;
  }
  std::optional<uint32_t> getMaxAclEntries() const override {
    // Max ACL entries per ACL table
    return 256;
  }
  std::optional<uint32_t> getMaxNdpTableSize() const override {
    return 20476;
  }
  std::optional<uint32_t> getMaxArpTableSize() const override {
    return 40944;
  }
};

} // namespace facebook::fboss
