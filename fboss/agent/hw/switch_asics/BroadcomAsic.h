// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

class BroadcomAsic : public HwAsic {
 public:
  std::string getVendor() const override {
    return "bcm";
  }
  uint32_t getMaxMirrors() const override {
    return 4;
  }
  uint32_t getSflowShimHeaderSize() const override {
    return 8;
  }
  std::optional<uint32_t> getPortSerdesPreemphasis() const override {
    return std::nullopt;
  }
};
} // namespace facebook::fboss
