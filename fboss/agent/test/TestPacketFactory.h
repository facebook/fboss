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

#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/MoveWrapper.h>
#include <folly/String.h>
#include <folly/io/IOBuf.h>

#include <memory>

namespace facebook::fboss {

class TxPacket;
class SwSwitch;

folly::IOBuf createV4Packet(
    folly::IPAddressV4 srcAddr,
    folly::IPAddressV4 dstAddr,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const std::string& payloadPad = "");

folly::IOBuf createV6Packet(
    const folly::IPAddressV6& srcAddr,
    const folly::IPAddressV6& dstAddr,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const std::string& payloadPad = "");

/* Convenience function to copy a buffer to a TxPacket.
 * L2 Header is advanced from RxPacket but headroom is provided in TxPacket.
 */
std::unique_ptr<TxPacket> createTxPacket(SwSwitch* sw, const folly::IOBuf& buf);

} // namespace facebook::fboss
