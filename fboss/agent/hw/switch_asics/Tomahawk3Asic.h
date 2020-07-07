// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/BroadcomAsic.h"

namespace facebook::fboss {

class Tomahawk3Asic : public BroadcomAsic {
  bool isSupported(Feature) const override;
  AsicType getAsicType() const override {
    return AsicType::ASIC_TYPE_TOMAHAWK3;
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
  int getDefaultNumPortQueues(cfg::StreamType streamType) const override {
    switch (streamType) {
      case cfg::StreamType::UNICAST:
        return 8;
      case cfg::StreamType::MULTICAST:
        return 4;
      case cfg::StreamType::ALL:
        throw FbossError("no queue exist for this stream type");
    }
    throw FbossError("Unknown streamType", streamType);
  }
  uint32_t getMaxLabelStackDepth() const override {
    return 9;
  }
  uint64_t getMMUSizeBytes() const override {
    return 64 * 1024 * 1024;
  }
};

} // namespace facebook::fboss
