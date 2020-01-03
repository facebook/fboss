/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/IPv6Hdr.h"

#include <folly/IPAddress.h>
#include <folly/IPAddressV6.h>
#include <folly/lang/Bits.h>
#include <sstream>
#include <stdexcept>
#include "fboss/agent/packet/PktUtil.h"

namespace facebook::fboss {

using folly::Endian;
using folly::IOBuf;
using folly::IPAddress;
using folly::IPAddressV6;
using folly::io::Cursor;
using std::string;
using std::stringstream;

IPv6Hdr::IPv6Hdr(Cursor& cursor) {
  try {
    uint8_t buf[16];
    cursor.pull(buf, 4);
    version = buf[0] >> 4;
    if (version != IPV6_VERSION) {
      throw HdrParseError("IPv6: version != 6");
    }
    trafficClass = ((buf[0] & 0x0F) << 4) | ((buf[1] & 0xF0) >> 4);
    flowLabel = (static_cast<uint32_t>(buf[1] & 0x0F) << 16) |
        (static_cast<uint32_t>(buf[2]) << 8) | static_cast<uint32_t>(buf[3]);
    payloadLength = cursor.readBE<uint16_t>();
    nextHeader = cursor.read<uint8_t>();
    hopLimit = cursor.read<uint8_t>();
    if (hopLimit == 0) {
      throw HdrParseError("IPv6: Hop Limit == 0");
    }
    srcAddr = PktUtil::readIPv6(&cursor);
    dstAddr = PktUtil::readIPv6(&cursor);
  } catch (const std::out_of_range& e) {
    throw HdrParseError("IPv6 header too small");
  }
}

void IPv6Hdr::serialize(folly::io::RWPrivateCursor* cursor) const {
  cursor->write<uint8_t>((version << 4) | (trafficClass >> 4));
  cursor->write<uint8_t>(
      ((trafficClass & 0xf) << 4) | ((flowLabel >> 16) & 0xf));
  cursor->writeBE<uint16_t>(flowLabel & 0xffff);
  cursor->writeBE<uint16_t>(payloadLength);
  cursor->write<uint8_t>(nextHeader);
  cursor->write<uint8_t>(hopLimit);
  cursor->push(srcAddr.bytes(), 16);
  cursor->push(dstAddr.bytes(), 16);
}

uint32_t IPv6Hdr::pseudoHdrPartialCsum() const {
  return pseudoHdrPartialCsum(payloadLength);
}

uint32_t IPv6Hdr::pseudoHdrPartialCsum(uint32_t length) const {
  uint32_t sum = 0;
  sum += addrPartialCsum(srcAddr);
  sum += addrPartialCsum(dstAddr);
  sum += (length >> 16);
  sum += (length & 0xffff);
  sum += nextHeader;
  return sum;
}

uint32_t IPv6Hdr::addrPartialCsum(const IPAddressV6& addr) {
  IOBuf buf(IOBuf::WRAP_BUFFER, addr.bytes(), 16);
  Cursor c(&buf);
  uint32_t sum = 0;
  for (uint8_t n = 0; n < 8; ++n) {
    sum += c.readBE<uint16_t>();
  }
  return sum;
}

string IPv6Hdr::toString() const {
  stringstream ss;
  ss << "IPv6Hdr { srcAddr: " << srcAddr.str() << " dstAddr: " << dstAddr.str()
     << " payloadLength: " << (int)payloadLength
     << " nextHeader: " << (int)nextHeader << " }";
  return ss.str();
}

} // namespace facebook::fboss
