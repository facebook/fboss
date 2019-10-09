// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook {
namespace fboss {

class FakeAsic : public HwAsic {
 public:
  bool isSupported(Feature) const override {
    return true; // fake supports all features
  }
};

} // namespace fboss
} // namespace facebook
