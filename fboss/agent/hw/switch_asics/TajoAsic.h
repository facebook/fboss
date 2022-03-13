// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

class TajoAsic : public HwAsic {
  bool isSupported(Feature) const override;
  std::string getVendor() const override {
    return "tajo";
  }
  uint64_t getMMUSizeBytes() const override {
    return 108 * 1024 * 1024;
  }
  uint32_t getMaxMirrors() const override {
    // TODO - verify this
    return 4;
  }
  uint32_t getSflowShimHeaderSize() const override {
    return 9;
  }
};

} // namespace facebook::fboss
