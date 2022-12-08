/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/ICMPHdr.h"

#include <stdexcept>

#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/HdrParseError.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/NDP.h"
#include "fboss/agent/packet/PktUtil.h"

namespace facebook::fboss {

using folly::IOBuf;
using folly::IPAddress;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;

ICMPHdr::ICMPHdr(Cursor& cursor) {
  try {
    type = cursor.read<uint8_t>();
    code = cursor.read<uint8_t>();
    csum = cursor.readBE<uint16_t>();
  } catch (const std::out_of_range& e) {
    throw HdrParseError("ICMPv6 header too small");
  }
}

void ICMPHdr::serialize(folly::io::RWPrivateCursor* cursor) const {
  cursor->write<uint8_t>(type);
  cursor->write<uint8_t>(code);
  cursor->writeBE<uint16_t>(csum);
}

uint16_t ICMPHdr::computeChecksum(const IPv6Hdr& ipv6Hdr, const Cursor& cursor)
    const {
  // The checksum is computed over the IPv6 pseudo header,
  // our header with the checksum set to 0, followed by the body.
  uint32_t sum = ipv6Hdr.pseudoHdrPartialCsum();
  // Checksum our header
  sum += ((type << 8) + code);

  // Checksum the body data.
  auto length = (ipv6Hdr.payloadLength - ICMPHdr::SIZE);
  return PktUtil::finalizeChecksum(cursor, length, sum);
}

uint16_t ICMPHdr::computeChecksum(const Cursor& cursor, uint32_t payloadLength)
    const {
  // Checksum our header
  uint32_t sum = 0;
  sum += ((type << 8) + code);

  // Checksum the body data.
  return PktUtil::finalizeChecksum(cursor, payloadLength, sum);
}

void ICMPHdr::serializePktHdr(
    folly::io::RWPrivateCursor* cursor,
    MacAddress dstMac,
    MacAddress srcMac,
    std::optional<VlanID> vlan,
    const IPv4Hdr& ipv4) {
  cursor->push(dstMac.bytes(), folly::MacAddress::SIZE);
  cursor->push(srcMac.bytes(), folly::MacAddress::SIZE);
  if (vlan.has_value()) {
    cursor->writeBE<uint16_t>(static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN));
    cursor->writeBE<uint16_t>(vlan.value());
  }
  cursor->writeBE<uint16_t>(static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4));

  DCHECK_EQ(ipv4.protocol, static_cast<uint8_t>(IP_PROTO::IP_PROTO_ICMP));
  ipv4.write(cursor);
}

uint32_t ICMPHdr::computeTotalLengthV4(uint32_t payloadLength) {
  return payloadLength + IPv4Hdr::minSize() + ICMPHdr::SIZE + EthHdr::SIZE;
}

void ICMPHdr::serializePktHdr(
    folly::io::RWPrivateCursor* cursor,
    MacAddress dstMac,
    MacAddress srcMac,
    std::optional<VlanID> vlan,
    const IPv6Hdr& ipv6,
    uint32_t payloadLength) {
  // TODO: clean up the EthHdr code and use EthHdr here
  cursor->push(dstMac.bytes(), folly::MacAddress::SIZE);
  cursor->push(srcMac.bytes(), folly::MacAddress::SIZE);
  if (vlan.has_value()) {
    cursor->writeBE<uint16_t>(static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN));
    cursor->writeBE<uint16_t>(vlan.value());
  }
  cursor->writeBE<uint16_t>(static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));

  DCHECK_EQ(ipv6.payloadLength, ICMPHdr::SIZE + payloadLength);
  DCHECK_EQ(
      ipv6.nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP));
  ipv6.serialize(cursor);
}

uint32_t ICMPHdr::computeTotalLengthV6(uint32_t payloadLength) {
  return payloadLength + ICMPHdr::SIZE + IPv6Hdr::SIZE + EthHdr::SIZE;
}

} // namespace facebook::fboss
