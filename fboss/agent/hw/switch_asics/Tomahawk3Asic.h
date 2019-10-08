// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook {
namespace fboss {

class Tomahawk3Asic : public HwAsic {
  bool isSupported(Feature) const override;
};

} // namespace fboss
} // namespace facebook
