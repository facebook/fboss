/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "UDPHeader.h"
#include <sstream>
#include <folly/io/Cursor.h>
#include <folly/lang/Bits.h>
#include "fboss/agent/PortStats.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktUtil.h"

using folly::io::Cursor;
using folly::Endian;
using std::string;
using std::stringstream;

namespace facebook { namespace fboss {

template<typename IPHDR>
uint16_t UDPHeader::computeChecksumImpl(const IPHDR& ip,
                                        const Cursor& cursor) const {
  // Checksum the IP pseudo-header
  uint32_t sum = ip.pseudoHdrPartialCsum();
  // Checksum the udp header
  sum += srcPort;
  sum += dstPort;
  sum += length;
  // Now the payload
  auto payloadLength = length - UDPHeader::size();

  // Finish the end-around carry
  uint16_t cs = PktUtil::finalizeChecksum(cursor, payloadLength, sum);
  // A 0 checksum should be transmitted as all ones
  if (cs == 0) {
    cs = 0xffff;
  }
  return cs;
}

void UDPHeader::parse(SwSwitch *sw, PortID port, Cursor* cursor) {
  try {
    parse(cursor);
  } catch (std::out_of_range& e) {
    sw->portStats(port)->udpTooSmall();
    throw FbossError("Too small packet. Got ", cursor->length(),
                     " bytes. Minimum ", size(), " bytes");
  }
}

void UDPHeader::parse(Cursor* cursor) {
  srcPort = cursor->readBE<uint16_t>();
  dstPort = cursor->readBE<uint16_t>();
  length = cursor->readBE<uint16_t>();
  csum = cursor->readBE<uint16_t>();
}

bool UDPHeader::operator==(const UDPHeader& r) const {
  return length == r.length &&
    csum == r.csum &&
    srcPort == r.srcPort &&
    dstPort == r.dstPort;
}

uint16_t UDPHeader::computeChecksum(const IPv4Hdr& ipv4Hdr,
                                    const Cursor& cursor) const {
  return computeChecksumImpl(ipv4Hdr, cursor);
}

void UDPHeader::updateChecksum(const IPv4Hdr& ipv6Hdr,
                               const Cursor& cursor) {
  csum = computeChecksum(ipv6Hdr, cursor);
}

uint16_t UDPHeader::computeChecksum(const IPv6Hdr& ipv6Hdr,
                                   const Cursor& cursor) const {
  return computeChecksumImpl(ipv6Hdr, cursor);
}

void UDPHeader::updateChecksum(const IPv6Hdr& ipv6Hdr, const Cursor& cursor) {
  csum = computeChecksum(ipv6Hdr, cursor);
}

string UDPHeader::toString() const {
  stringstream ss;
  ss << " Length: " << length
     << " Checksum: " << csum
     << " Source port: " << srcPort
     << " Destination port: " << dstPort;
  return ss.str();
}
}}
