/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/wedge/yamp/YampPlatformMapping.h"

#include "fboss/agent/platforms/wedge/yamp/Yamp16QPimPlatformMapping.h"

namespace facebook {
namespace fboss {
YampPlatformMapping::YampPlatformMapping() {
  // current Yamp platform only supports 16Q pims
  auto yamp16Q = std::make_unique<Yamp16QPimPlatformMapping>();
  for (uint8_t pimID = 2; pimID < 10; pimID++) {
    this->merge(yamp16Q->getPimPlatformMapping(pimID));
  }
}
} // namespace fboss
} // namespace facebook
