/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/wedge/minipack/MinipackPlatformMapping.h"

#include "fboss/agent/platforms/wedge/minipack/Minipack16QPimPlatformMapping.h"

namespace facebook {
namespace fboss {
MinipackPlatformMapping::MinipackPlatformMapping(
    ExternalPhyVersion xphyVersion) {
  // current Minipack oss platform only supports 16Q pims
  auto minipack16Q =
      std::make_unique<Minipack16QPimPlatformMapping>(xphyVersion);
  for (uint8_t pimID = 2; pimID < 10; pimID++) {
    this->merge(minipack16Q->getPimPlatformMapping(pimID));
  }
}
} // namespace fboss
} // namespace facebook
