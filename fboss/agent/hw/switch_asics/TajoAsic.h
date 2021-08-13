// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

class TajoAsic : public HwAsic {
  bool isSupported(Feature) const override;
  AsicType getAsicType() const override {
    return AsicType::ASIC_TYPE_TAJO;
  }
  std::string getVendor() const override {
    return "tajo";
  }
  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::HUNDREDG;
  }
  std::set<cfg::StreamType> getQueueStreamTypes(bool /* cpu */) const override {
    return {cfg::StreamType::ALL};
  }
  int getDefaultNumPortQueues(cfg::StreamType streamType, bool /*cpu*/)
      const override {
    switch (streamType) {
      case cfg::StreamType::UNICAST:
      case cfg::StreamType::MULTICAST:
        throw FbossError("no queue exist for this stream type");
      case cfg::StreamType::ALL:
        return 8;
    }

    throw FbossError("Unknown streamType", streamType);
  }
  uint32_t getMaxLabelStackDepth() const override {
    return 3;
  }
  uint64_t getMMUSizeBytes() const override {
    return 108 * 1024 * 1024;
  }
  uint32_t getMaxMirrors() const override {
    // TODO - verify this
    return 4;
  }
  uint64_t getDefaultReservedBytes(cfg::StreamType /*streamType*/, bool /*cpu*/)
      const override {
    // Concept of reserved bytes does not apply to Tajo
    return 0;
  }
  cfg::MMUScalingFactor getDefaultScalingFactor(
      cfg::StreamType /*streamType*/,
      bool /*cpu*/) const override {
    // Concept of scaling factor does not apply returning the same value TH3
    return cfg::MMUScalingFactor::TWO;
  }
  int getMaxNumLogicalPorts() const override {
    // 256 physical lanes + cpu
    return 257;
  }
  uint16_t getMirrorTruncateSize() const override {
    return 220;
  }
  uint32_t getMaxWideEcmpSize() const override {
    return 128;
  }
  int getSystemPortIDOffset() const override {
    return 1000;
  }
  uint32_t getSflowShimHeaderSize() const override {
    return 9;
  }
};

} // namespace facebook::fboss
