// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

class FakeAsic : public HwAsic {
 public:
  bool isSupported(Feature) const override {
    return true; // fake supports all features
  }
  AsicType getAsicType() const override {
    return AsicType::ASIC_TYPE_FAKE;
  }
  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::HUNDREDG;
  }
};

} // namespace facebook::fboss
