// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook {
namespace fboss {

class TomahawkAsic : public HwAsic {
 public:
  bool isSupported(Feature) const override;
};

} // namespace fboss
} // namespace facebook
