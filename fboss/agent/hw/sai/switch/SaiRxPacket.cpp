/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiRxPacket.h"

#include <folly/io/IOBuf.h>

namespace facebook::fboss {

void freeRxPacket(void* /* ptr */, void* /* arg */) {
  // The packet is on the stack, don't "free" it.
}

SaiRxPacket::SaiRxPacket(
    size_t buffer_size,
    const void* buffer,
    PortID portId,
    VlanID vlanId) {
  buf_ = folly::IOBuf::takeOwnership(
      const_cast<void*>(buffer), buffer_size, freeRxPacket, nullptr);
  len_ = buffer_size;
  srcPort_ = portId;
  srcVlan_ = vlanId;
}

} // namespace facebook::fboss
