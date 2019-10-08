// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/Trident2Asic.h"

namespace facebook {
namespace fboss {

bool Trident2Asic::isSupported(Feature) const {
  return false;
}

} // namespace fboss
} // namespace facebook
