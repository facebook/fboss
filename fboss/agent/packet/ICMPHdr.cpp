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
#include "fboss/agent/packet/PktUtil.h"

namespace facebook { namespace fboss {

using folly::IPAddress;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::IOBuf;
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

uint16_t ICMPHdr::computeChecksum(const IPv6Hdr& ipv6Hdr,
                                    const Cursor& cursor) const {
  // The checksum is computed over the IPv6 pseudo header,
  // our header with the checksum set to 0, followed by the body.
  uint32_t sum = ipv6Hdr.pseudoHdrPartialCsum();
  // Checksum our header
  sum += ((type << 8) + code);

  // Checksum the body data.
  auto length = (ipv6Hdr.payloadLength - ICMPHdr::SIZE);
  return PktUtil::finalizeChecksum(cursor, length, sum);
}

uint16_t ICMPHdr::computeChecksum(const Cursor& cursor,
                                  uint32_t payloadLength) const {
  // Checksum our header
  uint32_t sum = 0;
  sum += ((type << 8) + code);

  // Checksum the body data.
  return PktUtil::finalizeChecksum(cursor, payloadLength, sum);
}

void ICMPHdr::serializePktHdr(folly::io::RWPrivateCursor* cursor,
                                MacAddress dstMac,
                                MacAddress srcMac,
                                VlanID vlan,
                                const IPv4Hdr& ipv4) {
  cursor->push(dstMac.bytes(), folly::MacAddress::SIZE);
  cursor->push(srcMac.bytes(), folly::MacAddress::SIZE);
  cursor->writeBE<uint16_t>(static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN));
  cursor->writeBE<uint16_t>(vlan);
  cursor->writeBE<uint16_t>(static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4));

  DCHECK_EQ(ipv4.protocol, static_cast<uint8_t>(IP_PROTO::IP_PROTO_ICMP));
  ipv4.write(cursor);
}

uint32_t ICMPHdr::computeTotalLengthV4(uint32_t payloadLength) {
  return payloadLength + IPv4Hdr::minSize() + ICMPHdr::SIZE + EthHdr::SIZE;
}

void ICMPHdr::serializePktHdr(folly::io::RWPrivateCursor* cursor,
                                MacAddress dstMac,
                                MacAddress srcMac,
                                VlanID vlan,
                                const IPv6Hdr& ipv6,
                                uint32_t payloadLength) {
  // TODO: clean up the EthHdr code and use EthHdr here
  cursor->push(dstMac.bytes(), folly::MacAddress::SIZE);
  cursor->push(srcMac.bytes(), folly::MacAddress::SIZE);
  cursor->writeBE<uint16_t>(static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN));
  cursor->writeBE<uint16_t>(vlan);
  cursor->writeBE<uint16_t>(static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));

  DCHECK_EQ(ipv6.payloadLength, ICMPHdr::SIZE + payloadLength);
  DCHECK_EQ(
      ipv6.nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP));
  ipv6.serialize(cursor);
}

uint32_t ICMPHdr::computeTotalLengthV6(uint32_t payloadLength) {
  return payloadLength + ICMPHdr::SIZE + IPv6Hdr::SIZE + EthHdr::SIZE;
}

NDPOptionHdr::NDPOptionHdr(folly::io::Cursor& cursor) {
  try {
    type_ = static_cast<ICMPv6NDPOptionType>(cursor.read<uint8_t>());
    length_ = cursor.read<uint8_t>();
  } catch (const std::out_of_range& e) {
    throw HdrParseError("NDP Option header is not present");
  }
  if (length_ == 0) {
    throw HdrParseError("Invalid NDP Option header: length is 0");
  }
}

uint32_t NDPOptions::getMtu(folly::io::Cursor& cursor) {
  try {
    // This reserved field *must* be ignored by the receiver (RFC4861 4.6.4)
    cursor.skip(2);
    return cursor.readBE<uint32_t>();
  } catch (const std::out_of_range& e) {
    throw HdrParseError("NDP MTU option is too small");
  }
}
folly::MacAddress NDPOptions::getSourceLinkLayerAddress(
    folly::io::Cursor& cursor) {
  try {
    return PktUtil::readMac(&cursor);
  } catch (const std::out_of_range& e) {
    throw HdrParseError("NDP Source Link Layer option is too small");
  }
}

void NDPOptions::skipOption(
    const NDPOptionHdr& ndpHdr,
    folly::io::Cursor& cursor) {
  cursor += ndpHdr.payloadLength();
}

NDPOptions NDPOptions::getAll(folly::io::Cursor& cursor) {
  NDPOptions options;
  try {
    while (cursor.length()) {
      auto hdr = NDPOptionHdr(cursor);
      ICMPv6NDPOptionType hdr_type = hdr.type();
      XLOG(DBG4) << "NDP Option: " << static_cast<int>(hdr_type);
      if (!cursor.canAdvance(hdr.payloadLength())) {
        throw HdrParseError("NDP packet is too small");
      }
      switch (hdr_type) {
        case ICMPv6NDPOptionType::ICMPV6_NDP_OPTION_MTU:
          options.mtu.emplace(getMtu(cursor));
          break;
        case ICMPv6NDPOptionType::ICMPV6_NDP_OPTION_SOURCE_LINK_LAYER_ADDRESS:
          options.sourceLinkLayerAddress.emplace(
              getSourceLinkLayerAddress(cursor));
          break;
        case ICMPv6NDPOptionType::ICMPV6_NDP_OPTION_REDIRECTED_HEADER:
        case ICMPv6NDPOptionType::ICMPV6_NDP_OPTION_PREFIX_INFORMATION:
        case ICMPv6NDPOptionType::ICMPV6_NDP_OPTION_TARGET_LINK_LAYER_ADDRESS:
          XLOG(WARNING) << "Ignoring NDP Option: "
                        << static_cast<int>(hdr.type());
          skipOption(hdr, cursor);
          break;
        default:
          XLOG(WARNING) << "Ignoring unknown NDP Option: "
                        << static_cast<int>(hdr.type());
          skipOption(hdr, cursor);
          break;
      }
    }
  } catch (const HdrParseError& e) {
    XLOG(WARNING) << e.what();
  }
  return options;
}
}}
