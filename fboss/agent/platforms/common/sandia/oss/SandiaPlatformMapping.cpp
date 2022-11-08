/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/sandia/SandiaPlatformMapping.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/sandia/Sandia16QPimPlatformMapping.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {
SandiaPlatformMapping::SandiaPlatformMapping() {
  auto sandia16Q = std::make_unique<Sandia16QPimPlatformMapping>();
  for (uint8_t pimID = 2; pimID < 10; pimID++) {
    this->merge(sandia16Q->getPimPlatformMapping(pimID));
  }
}
} // namespace facebook::fboss
