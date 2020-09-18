// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/BroadcomAsic.h"

namespace facebook::fboss {

class Tomahawk4Asic : public BroadcomAsic {
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
  int getDefaultNumPortQueues(cfg::StreamType streamType) const override {
    // 12 logical queues in total, same as tomahawk3
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
    // one VC label and 8 tunnel labels, same as tomahawk3
    return 9;
  }
  uint64_t getMMUSizeBytes() const override {
    return 2 * 234606 * 254;
  }

  int getDefaultACLGroupID() const override;

  int getStationID(int intfId) const override;

  int getDefaultDropEgressID() const override;
};

} // namespace facebook::fboss
