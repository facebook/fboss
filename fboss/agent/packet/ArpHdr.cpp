/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/ArpHdr.h"

#include <stdexcept>

namespace facebook::fboss {

using folly::IPAddressV4;
using folly::MacAddress;
using folly::io::Cursor;

ArpHdr::ArpHdr(Cursor& cursor) {
  try {
    htype = cursor.readBE<uint16_t>();
    ptype = cursor.readBE<uint16_t>();
    hlen = cursor.read<uint8_t>();
    plen = cursor.read<uint8_t>();
    oper = cursor.readBE<uint16_t>();
    uint8_t macAddress[6];
    uint32_t ipAddress;
    cursor.pull(&macAddress, 6);
    sha = MacAddress::fromBinary(
        folly::ByteRange(&macAddress[0], MacAddress::SIZE));
    cursor.pull(&ipAddress, 4);
    spa = IPAddressV4::fromLong(ipAddress);
    cursor.pull(&macAddress, 6);
    tha = MacAddress::fromBinary(
        folly::ByteRange(&macAddress[0], MacAddress::SIZE));
    cursor.pull(&ipAddress, 4);
    tpa = IPAddressV4::fromLong(ipAddress);
  } catch (const std::out_of_range& e) {
    throw HdrParseError("ARP header too small");
  }
}

} // namespace facebook::fboss
