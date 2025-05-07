/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include <cstdint>

#include "fboss/agent/hw/test/dataplane_tests/HwTestAqmUtils.h"

extern "C" {
#include <bcm/cosq.h>
}

namespace facebook::fboss::utility {

int32_t HwTestThriftHandler::getEgressSharedPoolLimitBytes() {
  return utility::getEgressSharedPoolLimitBytes(hwSwitch_);
}

} // namespace facebook::fboss::utility
