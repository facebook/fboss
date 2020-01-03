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
class SwSwitch;
class PortStats;

struct UDPHeader {
 public:
  UDPHeader() : srcPort(0), dstPort(0), length(0), csum(0) {}
  UDPHeader(
      uint16_t _srcPort,
      uint16_t _dstPort,
      uint16_t _length,
      uint16_t _csum = 0)
      : srcPort(_srcPort), dstPort(_dstPort), length(_length), csum(_csum) {}

  bool operator==(const UDPHeader& r) const;
  /*
   * Accessors
   */
  static size_t size() {
    return 8;
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

  void parse(folly::io::Cursor* cursor, PortStats* stats);

  uint16_t computeChecksum(
      const IPv4Hdr& ipv4Hdr,
      const folly::io::Cursor& cursor) const;
  void updateChecksum(const IPv4Hdr& ipv4Hdr, const folly::io::Cursor& cursor);
  uint16_t computeChecksum(
      const IPv6Hdr& ipv6Hdr,
      const folly::io::Cursor& cursor) const;
  void updateChecksum(const IPv6Hdr& ipv6Hdr, const folly::io::Cursor& cursor);

  uint16_t srcPort;
  uint16_t dstPort;
  uint16_t length;
  uint16_t csum;

 private:
  void parse(folly::io::Cursor* cursor);
  template <typename IPHDR>
  uint16_t computeChecksumImpl(const IPHDR& ip, const folly::io::Cursor& cursor)
      const;
};

// toAppend to make folly::to<string> work with UDP header
inline void toAppend(const UDPHeader& udpHdr, std::string* result) {
  result->append(udpHdr.toString());
}

inline void toAppend(const UDPHeader& udpHdr, folly::fbstring* result) {
  result->append(udpHdr.toString());
}

inline std::ostream& operator<<(std::ostream& os, const UDPHeader& udpHdr) {
  os << udpHdr.toString();
  return os;
}

template <typename CursorType>
void UDPHeader::write(CursorType* cursor) const {
  cursor->template writeBE<uint16_t>(srcPort);
  cursor->template writeBE<uint16_t>(dstPort);
  cursor->template writeBE<uint16_t>(length);
  cursor->template writeBE<uint16_t>(csum);
}

} // namespace facebook::fboss
