/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/EthHdr.h"

#include <sstream>
#include <stdexcept>

namespace facebook::fboss {

using folly::MacAddress;
using folly::io::Cursor;
using std::string;
using std::stringstream;

EthHdr::EthHdr(Cursor& cursor) {
  try {
    uint8_t buf[14];
    cursor.pull(buf, sizeof(buf));
    dstAddr =
        MacAddress::fromBinary(folly::ByteRange(&buf[0], MacAddress::SIZE));
    srcAddr =
        MacAddress::fromBinary(folly::ByteRange(&buf[6], MacAddress::SIZE));
    etherType =
        (static_cast<uint16_t>(buf[12]) << 8) | static_cast<uint16_t>(buf[13]);
    // Look for VLAN tags.
    while (etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN) ||
           etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_QINQ)) {
      cursor.pull(buf, 4);
      uint32_t tag = (static_cast<uint32_t>(etherType) << 16) |
          (static_cast<uint32_t>(buf[0]) << 8) | static_cast<uint32_t>(buf[1]);
      vlanTags.push_back(VlanTag(tag));
      etherType =
          (static_cast<uint16_t>(buf[2]) << 8) | static_cast<uint16_t>(buf[3]);
    }
  } catch (const std::out_of_range& e) {
    throw HdrParseError("Ethernet header too small");
  }
}

string VlanTag::toString() const {
  stringstream ss;
  ss << " Protocol: " << std::hex << "0x" << tpid()
     << " priority : " << std::dec << (int)pcp()
     << " drop eligibility: " << (int)dei() << " vlan : " << vid();
  return ss.str();
}

string EthHdr::toString() const {
  stringstream ss;
  ss << " Source : " << getSrcMac().toString()
     << " Destination: " << getDstMac().toString()
     << " Ether type: " << std::hex << "0x" << getEtherType();
  ss << " VlanTags: {";
  for (auto vlanTag : vlanTags) {
    ss << folly::to<std::string>("{", vlanTag, "} ");
  }
  ss << "}";
  return ss.str();
}

} // namespace facebook::fboss
