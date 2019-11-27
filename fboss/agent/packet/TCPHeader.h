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

#include "fboss/agent/Utils.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/types.h"

namespace folly {
namespace io {
class Cursor;
}
} // namespace folly

namespace facebook::fboss {

class IPv4Hdr;
class IPv6Hdr;

struct TCPHeader {
 public:
  TCPHeader(uint16_t _srcPort, uint16_t _dstPort)
      : srcPort(_srcPort), dstPort(_dstPort) {}

  TCPHeader(
      uint16_t _srcPort,
      uint16_t _dstPort,
      uint32_t _seqNumber,
      uint32_t _ackNumber,
      uint8_t _flags,
      uint8_t _windowSize,
      uint16_t _urgentPointer)
      : srcPort(_srcPort),
        dstPort(_dstPort),
        sequenceNumber(_seqNumber),
        ackNumber(_ackNumber),
        flags(_flags),
        windowSize(_windowSize),
        urgentPointer(_urgentPointer) {}

  bool operator==(const TCPHeader& r) const;
  static uint32_t size() {
    return 20;
  }
  /*
   * Output as a string
   */
  std::string toString() const;
  /*
   * Write out to a buffer
   */
  template <typename CursorType>
  void write(CursorType* cursor) const;

  uint16_t computeChecksum(
      const IPv4Hdr& ipv4Hdr,
      const folly::io::Cursor& cursor) const;
  void updateChecksum(const IPv4Hdr& ipv4Hdr, const folly::io::Cursor& cursor);
  uint16_t computeChecksum(
      const IPv6Hdr& ipv6Hdr,
      const folly::io::Cursor& cursor) const;
  void updateChecksum(const IPv6Hdr& ipv6Hdr, const folly::io::Cursor& cursor);

  uint16_t srcPort{0};
  uint16_t dstPort{0};
  uint32_t sequenceNumber{0};
  uint32_t ackNumber{0};
  uint16_t dataOffsetAndFlags{5 << 12};
  uint8_t flags{0};
  uint16_t windowSize{0};
  uint16_t csum{0};
  uint16_t urgentPointer{0};

  // TODO support options
 private:
  template <typename IPHDR>
  uint16_t computeChecksumImpl(const IPHDR& ip, const folly::io::Cursor& cursor)
      const;
};

// toAppend to make folly::to<string> work with TCP header
inline void toAppend(const TCPHeader& tcpHdr, std::string* result) {
  result->append(tcpHdr.toString());
}

inline void toAppend(const TCPHeader& tcpHdr, folly::fbstring* result) {
  result->append(tcpHdr.toString());
}

inline std::ostream& operator<<(std::ostream& os, const TCPHeader& tcpHdr) {
  os << tcpHdr.toString();
  return os;
}

template <typename CursorType>
void TCPHeader::write(CursorType* cursor) const {
  cursor->template writeBE<uint16_t>(srcPort);
  cursor->template writeBE<uint16_t>(dstPort);
  cursor->template writeBE<uint32_t>(sequenceNumber);
  cursor->template writeBE<uint32_t>(ackNumber);
  cursor->template writeBE<uint16_t>(dataOffsetAndFlags);
  cursor->template writeBE<uint16_t>(windowSize);
  cursor->template writeBE<uint16_t>(csum);
  cursor->template writeBE<uint16_t>(urgentPointer);
}
} // namespace facebook::fboss
