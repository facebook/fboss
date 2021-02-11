// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/sai/SaiPlatformInit.h"

namespace facebook::fboss {

std::unique_ptr<SaiPlatform> getLEBPlatform(
    std::unique_ptr<PlatformProductInfo> /*productInfo*/) {
  return nullptr;
}

bool isLEB() {
  return false;
}

} // namespace facebook::fboss
