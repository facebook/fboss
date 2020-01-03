/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/NDPRouterAdvertisement.h"

#include <stdexcept>

namespace facebook::fboss {

using folly::io::Cursor;

NDPRouterAdvertisement::NDPRouterAdvertisement(Cursor& cursor) {
  try {
    curHopLimit = cursor.read<uint8_t>();
    flags = cursor.read<uint8_t>();
    routerLifetime = cursor.readBE<uint16_t>();
    reachableTime = cursor.readBE<uint32_t>();
    retransTimer = cursor.readBE<uint32_t>();
  } catch (const std::out_of_range& e) {
    throw HdrParseError("NDP Router Advertisement message too small");
  }
}

} // namespace facebook::fboss
