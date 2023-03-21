// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include <set>

namespace facebook::fboss {

class FakeAsic : public HwAsic {
 public:
  FakeAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      std::optional<cfg::Range64> systemPortRange)
      : HwAsic(
            switchType,
            switchId,
            systemPortRange,
            {cfg::SwitchType::NPU,
             cfg::SwitchType::VOQ,
             cfg::SwitchType::FABRIC}) {}
  bool isSupported(Feature feature) const override {
    switch (feature) {
      case Feature::HSDK:
      case Feature::OBJECT_KEY_CACHE:
      case Feature::RESOURCE_USAGE_STATS:
      case Feature::PKTIO:
      case Feature::WEIGHTED_NEXTHOPGROUP_MEMBER:
      case Feature::SAI_WEIGHTED_NEXTHOPGROUP_MEMBER:
      case Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER:
      case Feature::OBM_COUNTERS:
      case Feature::PTP_TC:
      case Feature::PTP_TC_PCS:
      case Feature::EGRESS_QUEUE_FLEX_COUNTER:
      case Feature::NON_UNICAST_HASH:
      // Can be removed once CS00012110063 is resolved
      case Feature::SAI_PORT_SPEED_CHANGE:
      case Feature::SAI_MPLS_LABEL_LOOKUP_FAIL_COUNTER: // TODO(pshaikh):
                                                        // support in fake
      case HwAsic::Feature::LINK_TRAINING:
      case HwAsic::Feature::SAI_PORT_VCO_CHANGE:
        return false;

      default:
        return true;
    }
  }
  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_FAKE;
  }
  std::string getVendor() const override {
    return "fake";
  }
  phy::DataPlanePhyChipType getDataPlanePhyChipType() const override {
    return phy::DataPlanePhyChipType::IPHY;
  }
  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::HUNDREDG;
  }
  std::set<cfg::StreamType> getQueueStreamTypes(
      cfg::PortType portType) const override {
    switch (portType) {
      case cfg::PortType::CPU_PORT:
        return {cfg::StreamType::MULTICAST};
      case cfg::PortType::INTERFACE_PORT:
      case cfg::PortType::RECYCLE_PORT:
        return {cfg::StreamType::UNICAST};
      case cfg::PortType::FABRIC_PORT:
        return {cfg::StreamType::FABRIC_TX};
    }
    throw FbossError(
        "Fake ASIC does not support:",
        apache::thrift::util::enumNameSafe(portType));
  }
  int getDefaultNumPortQueues(cfg::StreamType streamType, bool /*cpu*/)
      const override {
    return streamType == cfg::StreamType::UNICAST ? 8 : 10;
  }
  uint32_t getMaxLabelStackDepth() const override {
    // Copying TH3's max label stack depth
    return 9;
  }
  uint64_t getMMUSizeBytes() const override {
    // Fake MMU size
    return 64 * 1024 * 1024;
  }
  uint32_t getMaxMirrors() const override {
    return 4;
  }
  uint64_t getDefaultReservedBytes(cfg::StreamType /*streamType*/, bool cpu)
      const override {
    // Mimicking TH
    return cpu ? 1664 : 0;
  }
  cfg::MMUScalingFactor getDefaultScalingFactor(
      cfg::StreamType /*streamType*/,
      bool /*cpu*/) const override {
    // Mimicking TH
    return cfg::MMUScalingFactor::TWO;
  }
  int getMaxNumLogicalPorts() const override {
    return 134;
  }
  uint16_t getMirrorTruncateSize() const override {
    return 0;
  }
  uint32_t getMaxWideEcmpSize() const override {
    return 512;
  }
  uint32_t getMaxLagMemberSize() const override {
    return 256;
  }
  uint32_t getSflowShimHeaderSize() const override {
    return 0;
  }
  std::optional<uint32_t> getPortSerdesPreemphasis() const override {
    return std::nullopt;
  }
  uint32_t getPacketBufferUnitSize() const override {
    return 0;
  }
  uint32_t getPacketBufferDescriptorSize() const override {
    return 0;
  }
  uint32_t getMaxEcmpSize() const override {
    return 512;
  }
  AsicVendor getAsicVendor() const override {
    return HwAsic::AsicVendor::ASIC_VENDOR_FAKE;
  }
  uint32_t getMaxVariableWidthEcmpSize() const override {
    return 128;
  }
  uint32_t getNumCores() const override {
    return 0;
  }
  bool scalingFactorBasedDynamicThresholdSupported() const override {
    return true;
  }
  int getBufferDynThreshFromScalingFactor(
      cfg::MMUScalingFactor scalingFactor) const override {
    // Mimicking TH
    switch (scalingFactor) {
      case cfg::MMUScalingFactor::ONE:
        return 0;
      case cfg::MMUScalingFactor::EIGHT:
        return 3;
      case cfg::MMUScalingFactor::ONE_128TH:
        return -7;
      case cfg::MMUScalingFactor::ONE_64TH:
        return -6;
      case cfg::MMUScalingFactor::ONE_32TH:
        return -5;
      case cfg::MMUScalingFactor::ONE_16TH:
        return -4;
      case cfg::MMUScalingFactor::ONE_8TH:
        return -3;
      case cfg::MMUScalingFactor::ONE_QUARTER:
        return -2;
      case cfg::MMUScalingFactor::ONE_HALF:
        return -1;
      case cfg::MMUScalingFactor::TWO:
        return 1;
      case cfg::MMUScalingFactor::FOUR:
        return 2;
      case cfg::MMUScalingFactor::ONE_32768TH:
        // Unsupported
        throw FbossError(
            "Unsupported scaling factor : ",
            apache::thrift::util::enumNameSafe(scalingFactor));
    }
    CHECK(0) << "Should never get here";
    return -1;
  }
  uint32_t getStaticQueueLimitBytes() const override {
    return 0;
  }
  cfg::Range64 getReservedEncapIndexRange() const override {
    return makeRange(1000, 2000);
  }
  HwAsic::RecyclePortInfo getRecyclePortInfo() const override {
    return {
        .coreId = 0,
        .corePortIndex = 1,
        .speedMbps = 10000 // 10G
    };
  }
  uint32_t getNumMemoryBuffers() const override {
    return 0;
  }
};
} // namespace facebook::fboss
