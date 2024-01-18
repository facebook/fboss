/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>

namespace facebook::fboss {
class SwSwitch;
class TxPacket;

namespace utility {

std::unique_ptr<facebook::fboss::TxPacket> makeEthTxPacket(
    SwSwitch* sw,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    facebook::fboss::ETHERTYPE etherType,
    std::optional<std::vector<uint8_t>> payload);
}
} // namespace facebook::fboss
