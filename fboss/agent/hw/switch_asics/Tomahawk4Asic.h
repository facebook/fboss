// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/BroadcomAsic.h"

namespace facebook::fboss {

class Tomahawk4Asic : public BroadcomAsic {
 public:
  bool isSupported(Feature) const override;
  AsicType getAsicType() const override {
    return AsicType::ASIC_TYPE_TOMAHAWK4;
  }
  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::FOURHUNDREDG;
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
    // one VC label and 8 tunnel labels, same as tomahawk3
    return 9;
  }
  uint64_t getMMUSizeBytes() const override {
    return 2 * 234606 * 254;
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
};

} // namespace facebook::fboss
