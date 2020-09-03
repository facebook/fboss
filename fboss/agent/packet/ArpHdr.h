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
 * Address Resolution Protocol
 *
 * Resolve network layer addresses (IP) into link layer addresses (Ethernet).
 *
 * References:
 *   https://en.wikipedia.org/wiki/Address_Resolution_Protocol
 *   https://tools.ietf.org/html/rfc826
 *   https://tools.ietf.org/html/rfc5227
 *   https://tools.ietf.org/html/rfc5494
 */
#include <folly/IPAddressV4.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>
#include "fboss/agent/packet/HdrParseError.h"

namespace facebook::fboss {

enum class ARP_HTYPE : uint16_t {
  ARP_HTYPE_ETHERNET = 1,
};

enum class ARP_PTYPE : uint16_t {
  ARP_PTYPE_IPV4 = 0x0800,
};

enum class ARP_HLEN : uint8_t {
  ARP_HLEN_ETHERNET = 6,
};

enum class ARP_PLEN : uint8_t {
  ARP_PLEN_IPV4 = 4,
};

enum class ARP_OPER : uint16_t {
  ARP_OPER_REQUEST = 1,
  ARP_OPER_REPLY = 2,
};

/*
 * An Address Resolution Protocol Header. The payload is not represented here.
 */
struct ArpHdr {
 public:
  /*
   * default constructor
   */
  ArpHdr() {}
  /*
   * copy constructor
   */
  ArpHdr(const ArpHdr& rhs)
      : htype(rhs.htype),
        ptype(rhs.ptype),
        hlen(rhs.hlen),
        plen(rhs.plen),
        oper(rhs.oper),
        sha(rhs.sha),
        spa(rhs.spa),
        tha(rhs.tha),
        tpa(rhs.tpa) {}
  /*
   * parameterized data constructor
   */
  ArpHdr(
      uint16_t _htype,
      uint16_t _ptype,
      uint8_t _hlen,
      uint8_t _plen,
      uint16_t _oper,
      const folly::MacAddress& _sha,
      const folly::IPAddressV4& _spa,
      const folly::MacAddress& _tha,
      const folly::IPAddressV4& _tpa)
      : htype(_htype),
        ptype(_ptype),
        hlen(_hlen),
        plen(_plen),
        oper(_oper),
        sha(_sha),
        spa(_spa),
        tha(_tha),
        tpa(_tpa) {}
  /*
   * cursor data constructor
   */
  explicit ArpHdr(folly::io::Cursor& cursor);
  /*
   * destructor
   */
  ~ArpHdr() {}
  /*
   * operator=
   */
  ArpHdr& operator=(const ArpHdr& rhs) {
    htype = rhs.htype;
    ptype = rhs.ptype;
    hlen = rhs.hlen;
    plen = rhs.plen;
    oper = rhs.oper;
    sha = rhs.sha;
    spa = rhs.spa;
    tha = rhs.tha;
    tpa = rhs.tpa;
    return *this;
  }

  /*
   * ARP Header size
   */
  static uint32_t size() {
    return 28;
  }

 public:
  /*
   * Hardware Type
   */
  uint16_t htype{0};
  /*
   * Protocol Type
   */
  uint16_t ptype{0};
  /*
   * Hardware Address Length, usually 6 for Ethernet
   */
  uint8_t hlen{0};
  /*
   * Protocol Address Length, usually 4 for IPv4
   */
  uint8_t plen{0};
  /*
   * Operation: 1 for request, 2 for reply
   */
  uint16_t oper{0};
  /*
   * Sender Hardware Address
   */
  folly::MacAddress sha;
  /*
   * Sender Protocol Address
   */
  folly::IPAddressV4 spa{"0.0.0.0"};
  /*
   * Target Hardware Address
   */
  folly::MacAddress tha;
  /*
   * Target Protocol Address
   */
  folly::IPAddressV4 tpa{"0.0.0.0"};
};

inline bool operator==(const ArpHdr& lhs, const ArpHdr& rhs) {
  return lhs.htype == rhs.htype && lhs.ptype == rhs.ptype &&
      lhs.hlen == rhs.hlen && lhs.plen == rhs.plen && lhs.oper == rhs.oper &&
      lhs.sha == rhs.sha && lhs.spa == rhs.spa && lhs.tha == rhs.tha &&
      lhs.tpa == rhs.tpa;
}

inline bool operator!=(const ArpHdr& lhs, const ArpHdr& rhs) {
  return !operator==(lhs, rhs);
}

} // namespace facebook::fboss
