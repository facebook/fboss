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

#include <sstream>
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
  } catch (const std::out_of_range&) {
    throw HdrParseError("ARP header too small");
  }
}

void ArpHdr::serialize(folly::io::RWPrivateCursor* cursor) const {
  cursor->writeBE<uint16_t>(htype);
  cursor->writeBE<uint16_t>(ptype);
  cursor->write<uint8_t>(hlen);
  cursor->write<uint8_t>(plen);
  cursor->writeBE<uint16_t>(oper);
  cursor->push(sha.bytes(), MacAddress::SIZE);
  cursor->push(spa.bytes(), IPAddressV4::byteCount());
  cursor->push(tha.bytes(), MacAddress::SIZE);
  cursor->push(tpa.bytes(), IPAddressV4::byteCount());
}

std::string ArpHdr::toString() const {
  std::stringstream ss;
  ss << " header type: " << htype << " protocol type: " << ptype
     << " header len: " << (int)hlen << " protocol len: " << (int)plen
     << "smac: " << sha << "src ip: " << spa << " dmac: " << tha
     << " dst ip: " << tpa;
  return ss.str();
}
} // namespace facebook::fboss
