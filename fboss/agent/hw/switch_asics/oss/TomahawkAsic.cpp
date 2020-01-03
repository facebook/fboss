// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"

namespace facebook::fboss {

bool TomahawkAsic::isSupported(Feature) const {
  return false;
}

} // namespace facebook::fboss
