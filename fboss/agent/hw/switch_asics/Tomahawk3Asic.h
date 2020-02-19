// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

class Tomahawk3Asic : public HwAsic {
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
};

} // namespace facebook::fboss
