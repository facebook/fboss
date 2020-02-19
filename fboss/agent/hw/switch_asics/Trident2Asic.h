// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

class Trident2Asic : public HwAsic {
 public:
  bool isSupported(Feature) const override;
  AsicType getAsicType() const override {
    return AsicType::ASIC_TYPE_TRIDENT2;
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
};

} // namespace facebook::fboss
