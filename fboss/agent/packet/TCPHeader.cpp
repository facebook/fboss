/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "TCPHeader.h"

#include <folly/io/Cursor.h>
#include <folly/lang/Bits.h>
#include <sstream>

#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktUtil.h"

using folly::Endian;
using folly::io::Cursor;
using std::string;
using std::stringstream;

namespace facebook::fboss {

template <typename IPHDR>
uint16_t TCPHeader::computeChecksumImpl(const IPHDR& ip, const Cursor& cursor)
    const {
  // Checksum the IP pseudo-header
  uint32_t sum = ip.pseudoHdrPartialCsum();
  // Checksum the tcp header
  sum += srcPort;
  sum += dstPort;
  sum += (sequenceNumber >> 16) + (sequenceNumber & 0xffff);
  sum += (ackNumber >> 16) + (ackNumber & 0xffff);
  sum += dataOffsetAndFlags;
  sum += windowSize;
  sum += urgentPointer;
  // Now the payload
  auto payloadLength = ip.payloadSize() - TCPHeader::size();

  // Finish the end-around carry
  uint16_t cs = PktUtil::finalizeChecksum(cursor, payloadLength, sum);
  // A 0 checksum should be transmitted as all ones
  if (cs == 0) {
    cs = 0xffff;
  }
  return cs;
}

bool TCPHeader::operator==(const TCPHeader& r) const {
  return std::tie(
             srcPort,
             dstPort,
             sequenceNumber,
             ackNumber,
             dataOffsetAndFlags,
             windowSize,
             csum,
             urgentPointer) ==
      std::tie(
             r.srcPort,
             r.dstPort,
             r.sequenceNumber,
             r.ackNumber,
             dataOffsetAndFlags,
             r.windowSize,
             r.csum,
             r.urgentPointer);
}

uint16_t TCPHeader::computeChecksum(
    const IPv4Hdr& ipv4Hdr,
    const Cursor& cursor) const {
  return computeChecksumImpl(ipv4Hdr, cursor);
}

void TCPHeader::updateChecksum(const IPv4Hdr& ipv6Hdr, const Cursor& cursor) {
  csum = computeChecksum(ipv6Hdr, cursor);
}

uint16_t TCPHeader::computeChecksum(
    const IPv6Hdr& ipv6Hdr,
    const Cursor& cursor) const {
  return computeChecksumImpl(ipv6Hdr, cursor);
}

void TCPHeader::updateChecksum(const IPv6Hdr& ipv6Hdr, const Cursor& cursor) {
  csum = computeChecksum(ipv6Hdr, cursor);
}

string TCPHeader::toString() const {
  stringstream ss;
  ss << " Source port: " << srcPort << " Destination port: " << dstPort
     << " Seq Num: " << sequenceNumber << " Ack Num: " << ackNumber
     << " Data offset and flags: " << dataOffsetAndFlags
     << " Checksum: " << csum << " Urgent pointer: " << urgentPointer;
  return ss.str();
}
} // namespace facebook::fboss
