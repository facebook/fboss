/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/PacketTestUtils.h"

#include "fboss/agent/LldpManager.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"

namespace facebook::fboss::utility {

std::unique_ptr<facebook::fboss::TxPacket> makeLLDPPacket(
    const SwSwitch* sw,
    const folly::MacAddress srcMac,
    std::optional<VlanID> vlanid,
    const std::string& hostname,
    const std::string& portname,
    const std::string& portdesc,
    const uint16_t ttl,
    const uint16_t capabilities) {
  return LldpManager::createLldpPkt(
      makeAllocator(sw),
      srcMac,
      vlanid,
      hostname,
      portname,
      portdesc,
      ttl,
      capabilities);
}

} // namespace facebook::fboss::utility
