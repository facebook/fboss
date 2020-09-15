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
  int getDefaultNumPortQueues(cfg::StreamType streamType) const override {
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
    return 64 * 1024 * 1024;
  }
  uint32_t getMaxMirrors() const override {
    // TODO - verify this
    return 4;
  }
};

} // namespace facebook::fboss
