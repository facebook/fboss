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
 * Internet Protocol Version 6
 *
 * References:
 *   https://en.wikipedia.org/wiki/IPv6
 *   https://en.wikipedia.org/wiki/IPv6_packet
 *   https://en.wikipedia.org/wiki/IPv6_address
 */
#include <folly/IPAddressV6.h>
#include <folly/io/Cursor.h>
#include "fboss/agent/packet/HdrParseError.h"
#include "fboss/agent/packet/IPProto.h"

namespace facebook::fboss {

enum : uint8_t {
  IPV6_VERSION = 6,
};

/*
 * An IPv6 Header. The payload is not represented here.
 *
 * TODO: Add support for IPv6 extension headers.
 */
class IPv6Hdr {
 public:
  enum { SIZE = 40 };

  /*
   * default constructor
   */
  IPv6Hdr() {}
  /*
   * copy constructor
   */
  IPv6Hdr(const IPv6Hdr& rhs)
      : version(rhs.version),
        trafficClass(rhs.trafficClass),
        flowLabel(rhs.flowLabel),
        payloadLength(rhs.payloadLength),
        nextHeader(rhs.nextHeader),
        hopLimit(rhs.hopLimit),
        srcAddr(rhs.srcAddr),
        dstAddr(rhs.dstAddr) {}
  /*
   * parameterized data constructor
   */
  IPv6Hdr(
      uint8_t _version,
      uint8_t _trafficClass,
      uint16_t _flowLabel,
      uint16_t _payloadLength,
      uint8_t _nextHeader,
      uint8_t _hopLimit,
      const folly::IPAddressV6& _srcAddr,
      const folly::IPAddressV6& _dstAddr)
      : version(_version),
        trafficClass(_trafficClass),
        flowLabel(_flowLabel),
        payloadLength(_payloadLength),
        nextHeader(_nextHeader),
        hopLimit(_hopLimit),
        srcAddr(_srcAddr),
        dstAddr(_dstAddr) {}

  IPv6Hdr(folly::IPAddressV6 src, folly::IPAddressV6 dst)
      : srcAddr(src), dstAddr(dst) {}

  /*
   * cursor data constructor
   */
  explicit IPv6Hdr(folly::io::Cursor& cursor);
  /*
   * destructor
   */
  ~IPv6Hdr() {}
  /*
   * operator=
   */
  IPv6Hdr& operator=(const IPv6Hdr& rhs) {
    version = rhs.version;
    trafficClass = rhs.trafficClass;
    flowLabel = rhs.flowLabel;
    payloadLength = rhs.payloadLength;
    nextHeader = rhs.nextHeader;
    hopLimit = rhs.hopLimit;
    srcAddr = rhs.srcAddr;
    dstAddr = rhs.dstAddr;
    return *this;
  }

  /*
   * Serialize the IPv6 header using the specified cursor.
   *
   * The cursor will be updated to point just after the IPv6 header.
   */
  void serialize(folly::io::RWPrivateCursor* cursor) const;

  /*
   * Compute the partial checksum over the IPv6 pseudo-header.
   *
   * This is used for upper-layer protocols that include the pseudo-header in
   * their own checksum.
   *
   * The optional length field is the payload length, if provided by the
   * upper-layer protocol header.  If not specified, the payloadLength from the
   * IPv6 header will be used.
   *
   * This returns the sum of the uint16_t values in the pseudo-header.
   * End-arround carry still needs to be applied.
   */
  uint32_t pseudoHdrPartialCsum() const;
  uint32_t pseudoHdrPartialCsum(uint32_t length) const;
  std::string toString() const;

  size_t static size() {
    return SIZE;
  }
  size_t payloadSize() const {
    return payloadLength;
  }

 public:
  /*
   * Always 0x6
   */
  uint8_t version{6};
  /*
   * Combination of DiffServ and ECN
   */
  uint8_t trafficClass{0};
  /*
   * Flow Label
   */
  uint32_t flowLabel{0};
  /*
   * Size of payload in octets, including IPv6 extension headers.
   * Set to zero when a Hop-by-Hop extension header carries a Jumbo Payload
   * option.
   */
  uint16_t payloadLength{0};
  /*
   * Same as IPv4 protocol field
   */
  uint8_t nextHeader{0};
  /*
   * Same as IPv4 TTL field
   */
  uint8_t hopLimit{255};
  /*
   * Source Address
   */
  folly::IPAddressV6 srcAddr;
  /*
   * Destination Address
   */
  folly::IPAddressV6 dstAddr;

 private:
  static uint32_t addrPartialCsum(const folly::IPAddressV6& addr);
};

inline bool operator==(const IPv6Hdr& lhs, const IPv6Hdr& rhs) {
  return lhs.version == rhs.version && lhs.trafficClass == rhs.trafficClass &&
      lhs.flowLabel == rhs.flowLabel &&
      lhs.payloadLength == rhs.payloadLength &&
      lhs.nextHeader == rhs.nextHeader && lhs.hopLimit == rhs.hopLimit &&
      lhs.srcAddr == rhs.srcAddr && lhs.dstAddr == rhs.dstAddr;
}

inline bool operator!=(const IPv6Hdr& lhs, const IPv6Hdr& rhs) {
  return !operator==(lhs, rhs);
}

} // namespace facebook::fboss
