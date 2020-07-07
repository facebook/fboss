// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/BroadcomAsic.h"

namespace facebook::fboss {

class TomahawkAsic : public BroadcomAsic {
 public:
  bool isSupported(Feature) const override;
  AsicType getAsicType() const override {
    return AsicType::ASIC_TYPE_TOMAHAWK;
  }
  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::HUNDREDG;
  }
  std::set<cfg::StreamType> getQueueStreamTypes(bool cpu) const override {
    if (cpu) {
      return {cfg::StreamType::MULTICAST};
    } else {
      return {cfg::StreamType::UNICAST};
    }
  }
  int getDefaultNumPortQueues(cfg::StreamType streamType) const override {
    switch (streamType) {
      case cfg::StreamType::UNICAST:
        return 8;
      case cfg::StreamType::MULTICAST:
        return 10;
      case cfg::StreamType::ALL:
        throw FbossError("no queue exist for this stream type");
    }
    throw FbossError("Unknown streamType", streamType);
  }
  uint32_t getMaxLabelStackDepth() const override {
    return 3;
  }
  uint64_t getMMUSizeBytes() const override {
    return 16 * 1024 * 1024;
  }
};

} // namespace facebook::fboss
