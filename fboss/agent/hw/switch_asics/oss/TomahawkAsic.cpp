// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"

namespace facebook {
namespace fboss {

bool TomahawkAsic::isSupported(Feature) const {
  return false;
}

} // namespace fboss
} // namespace facebook
