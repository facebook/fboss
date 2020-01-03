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

/*
 * Internet Protocol Version 4
 *
 * References:
 *   https://en.wikipedia.org/wiki/IPv4
 */
#include <folly/IPAddressV4.h>
#include <folly/io/Cursor.h>
#include <ostream>
#include "fboss/agent/packet/HdrParseError.h"
#include "fboss/agent/packet/IPProto.h"

namespace facebook::fboss {

enum : uint8_t {
  IPV4_VERSION = 4,
};

/*
 * An IPv4 Header. The payload is not represented here.
 */
class IPv4Hdr {
 public:
  /*
   * default constructor
   */
  IPv4Hdr() {}
  /*
   * copy constructor
   */
  IPv4Hdr(const IPv4Hdr& rhs)
      : version(rhs.version),
        ihl(rhs.ihl),
        dscp(rhs.dscp),
        ecn(rhs.ecn),
        length(rhs.length),
        id(rhs.id),
        dontFragment(rhs.dontFragment),
        moreFragments(rhs.moreFragments),
        fragmentOffset(rhs.fragmentOffset),
        ttl(rhs.ttl),
        protocol(rhs.protocol),
        csum(rhs.csum),
        srcAddr(rhs.srcAddr),
        dstAddr(rhs.dstAddr) {}

  /*
   * parameterized data constructor
   */
  IPv4Hdr(
      uint8_t _version,
      uint8_t _ihl,
      uint8_t _dscp,
      uint8_t _ecn,
      uint16_t _length,
      uint16_t _id,
      bool _dontFragment,
      bool _moreFragments,
      uint16_t _fragmentOffset,
      uint8_t _ttl,
      uint8_t _protocol,
      uint16_t _csum,
      const folly::IPAddressV4& _srcAddr,
      const folly::IPAddressV4& _dstAddr)
      : version(_version),
        ihl(_ihl),
        dscp(_dscp),
        ecn(_ecn),
        length(_length),
        id(_id),
        dontFragment(_dontFragment),
        moreFragments(_moreFragments),
        fragmentOffset(_fragmentOffset),
        ttl(_ttl),
        protocol(_protocol),
        csum(_csum),
        srcAddr(_srcAddr),
        dstAddr(_dstAddr) {}

  IPv4Hdr(
      const folly::IPAddressV4& _srcAddr,
      const folly::IPAddressV4& _dstAddr,
      uint8_t _protocol,
      uint16_t _bodyLength);

  /*
   * cursor data constructor
   */
  explicit IPv4Hdr(folly::io::Cursor& cursor);
  /*
   * destructor
   */
  ~IPv4Hdr() {}
  /*
   * operator=
   */
  IPv4Hdr& operator=(const IPv4Hdr& rhs) {
    version = rhs.version;
    ihl = rhs.ihl;
    dscp = rhs.dscp;
    ecn = rhs.ecn;
    length = rhs.length;
    id = rhs.id;
    dontFragment = rhs.dontFragment;
    moreFragments = rhs.moreFragments;
    fragmentOffset = rhs.fragmentOffset;
    ttl = rhs.ttl;
    protocol = rhs.protocol;
    csum = rhs.csum;
    srcAddr = rhs.srcAddr;
    ;
    dstAddr = rhs.dstAddr;
    return *this;
  }

  void computeChecksum();
  template <typename CursorType>
  void write(CursorType* cursor) const;
  template <typename CursorType>
  void serialize(CursorType* cursor) const {
    write(cursor);
  }

  /*
   * Compute the partial checksum over the IPv4 pseudo-header.
   *
   * This is used for upper-layer protocols that include the pseudo-header in
   * their own checksum (particularly UDP and TCP).
   *
   * This returns the sum of the uint16_t values in the pseudo-header.
   * End-arround carry still needs to be applied.
   */
  uint32_t pseudoHdrPartialCsum() const;
  uint32_t pseudoHdrPartialCsum(uint32_t payloadLength) const;

  /*
   * Output as a string
   */
  std::string toString() const;

  /*
   * Accessors
   */
  size_t static minSize() {
    return 20;
  }
  size_t size() const {
    return ihl * 4;
  }
  size_t payloadSize() const {
    return length - size();
  }
  /*
   * Always 0x4
   */
  uint8_t version{0};
  /*
   * Internet Header Length: number of 32-bit words in the header (RFC 791)
   *   Min:  5 (20 octets)
   *   Max: 15 (60 octets)
   */
  uint8_t ihl{0};
  /*
   * Differentiated Services Code Point (RFC 2474)
   */
  uint8_t dscp{0};
  /*
   * Explicit Congestion Notification (RFC 3168)
   */
  uint8_t ecn{0};
  /*
   * Entire packet (fragment) size in octets, including header and payload
   *   Min: 20 (just an IPv4 header)
   *   Max: 65,635
   */
  uint16_t length{0};
  /*
   * Used for uniquely identifying the group of fragments of a single IP
   * datagram.
   */
  uint16_t id{0};
  /*
   * Don't Fragment (DF)
   */
  bool dontFragment{false};
  /*
   * More Fragments (MF)
   */
  bool moreFragments{false};
  /*
   * In units of 8-octet blocks, specifies the offset of a particular fragment
   * relative to the beginning of the original unfragmented IP datagram.
   */
  uint16_t fragmentOffset{0};
  /*
   * Time To Live
   */
  uint8_t ttl{0};
  /*
   * Magic number specifying the encapsulated protocol (Layer 4)
   */
  uint8_t protocol{0};
  /*
   * Header Checksum
   */
  uint16_t csum{0};
  /*
   * Source Address
   */
  folly::IPAddressV4 srcAddr{"0.0.0.0"};
  /*
   * Destination Address
   */
  folly::IPAddressV4 dstAddr{"0.0.0.0"};

  // There might be up to (15 - 5) * 4 = 40 extra option bytes
  uint8_t optionBuf[40] = {};

 private:
  static uint32_t addrPartialCsum(const folly::IPAddressV4& addr);
};

inline bool operator==(const IPv4Hdr& lhs, const IPv4Hdr& rhs) {
  return lhs.version == rhs.version && lhs.ihl == rhs.ihl &&
      lhs.dscp == rhs.dscp && lhs.ecn == rhs.ecn && lhs.length == rhs.length &&
      lhs.id == rhs.id && lhs.dontFragment == rhs.dontFragment &&
      lhs.moreFragments == rhs.moreFragments &&
      lhs.fragmentOffset == rhs.fragmentOffset && lhs.ttl == rhs.ttl &&
      lhs.protocol == rhs.protocol && lhs.csum == rhs.csum &&
      lhs.srcAddr == rhs.srcAddr && lhs.dstAddr == rhs.dstAddr;
}

inline bool operator!=(const IPv4Hdr& lhs, const IPv4Hdr& rhs) {
  return !operator==(lhs, rhs);
}

template <typename CursorType>
void IPv4Hdr::write(CursorType* cursor) const {
  cursor->template write<uint8_t>(version << 4 | ihl);
  cursor->template write<uint8_t>(dscp << 2 | ecn);
  cursor->template writeBE<uint16_t>(length);
  cursor->template writeBE<uint16_t>(id);
  uint16_t flagsAndOffset = fragmentOffset;
  flagsAndOffset |= (dontFragment ? (uint16_t)1 << 14 : 0);
  flagsAndOffset |= (moreFragments ? (uint16_t)1 << 15 : 0);
  cursor->template writeBE<uint16_t>(flagsAndOffset);
  cursor->template write<uint8_t>(ttl);
  cursor->template write<uint8_t>(protocol);
  cursor->template writeBE<uint16_t>(csum);
  cursor->template write<uint32_t>(srcAddr.toLong());
  cursor->template write<uint32_t>(dstAddr.toLong());

  // If extra options are present, serialize them back blindly
  if (UNLIKELY(ihl > 5)) {
    CHECK_LE(ihl, 15) << "Corrupted ihl value";
    cursor->push(optionBuf, (ihl - 5) * 4);
  }
}

// toAppend to make folly::to<string> work with IPv4Hdr
inline void toAppend(const IPv4Hdr& ipv4Hdr, std::string* result) {
  result->append(ipv4Hdr.toString());
}

inline void toAppend(const IPv4Hdr& ipv4Hdr, folly::fbstring* result) {
  result->append(ipv4Hdr.toString());
}

inline std::ostream& operator<<(std::ostream& os, const IPv4Hdr& ipv4Hdr) {
  os << ipv4Hdr.toString();
  return os;
}

} // namespace facebook::fboss
