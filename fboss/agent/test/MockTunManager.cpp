/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/MockTunManager.h"

#include "fboss/agent/RxPacket.h"

namespace facebook::fboss {

MockTunManager::MockTunManager(SwSwitch* sw, folly::EventBase* evb)
    : TunManager(sw, evb) {}

bool MockTunManager::sendPacketToHost(
    InterfaceID dstIfID,
    std::unique_ptr<RxPacket> pkt) {
  std::shared_ptr<RxPacket> sp(pkt.release());
  sendPacketToHost_(std::make_tuple(dstIfID, sp));
  return true;
}

} // namespace facebook::fboss
