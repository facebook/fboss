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
 * Ethernet
 *
 * References:
 *   http://standards.ieee.org/develop/regauth/ethertype/eth.txt
 *   http://www.networksorcery.com/enp/protocol/802/ethertypes.htm
 *   https://en.wikipedia.org/wiki/EtherType
 */
#include <folly/Conv.h>
#include <folly/MacAddress.h>
#include <folly/String.h>
#include <folly/io/Cursor.h>
#include <ostream>
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/HdrParseError.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

/*
 * A 32-bit Ethernet VLAN tag.
 *
 * References:
 *   https://en.wikipedia.org/wiki/IEEE_802.1Q
 *   https://en.wikipedia.org/wiki/802.1ad
 */
class VlanTag {
 public:
  /*
   * default constructor
   */
  VlanTag() = default;
  /*
   * copy constructor
   */
  VlanTag(const VlanTag& rhs) = default;
  /*
   * data contstructor
   */
  explicit VlanTag(uint32_t tag) : value(tag) {}
  /*
   * Data constructor with individual members
   */
  VlanTag(
      uint16_t vlan,
      uint16_t protocol,
      uint8_t dropEligibility = 0,
      uint8_t priority = 0)
      : value(
            (protocol << 16) | ((priority & 0x7) << 13) |
            ((dropEligibility & 0x1) << 12) | (vlan & 0xfff)) {}
  /*
   * destructor
   */
  ~VlanTag() = default;
  /*
   * operator=
   */
  VlanTag& operator=(uint32_t tag) {
    value = tag;
    return *this;
  }
  VlanTag& operator=(const VlanTag& rhs) = default;

  /*
   * Tag Protocol Identifier: 0x8100 or 0x88a8
   */
  const uint16_t tpid() const {
    return (value & 0xFFFF0000) >> 16;
  }
  /*
   * Priority Code Point (801.1p priority)
   */
  const uint8_t pcp() const {
    return (value & 0x0000E000) >> 13;
  }
  /*
   * Drop Eligible Indicator
   */
  const uint8_t dei() const {
    return (value & 0x00001000) >> 12;
  }
  /*
   * VLAN Identifier
   */
  const uint16_t vid() const {
    return value & 0x00000FFF;
  }

  std::string toString() const;

 public:
  /*
   * value is stored in host byte order
   */
  uint32_t value{0};
};

// toAppend to make folly::to<string> work with VlanTag
inline void toAppend(VlanTag vlan, std::string* result) {
  result->append(vlan.toString());
}

inline void toAppend(VlanTag vlan, folly::fbstring* result) {
  result->append(vlan.toString());
}

inline bool operator==(const VlanTag& lhs, const VlanTag& rhs) {
  return lhs.value == rhs.value;
}

inline bool operator!=(const VlanTag& lhs, const VlanTag& rhs) {
  return !operator==(lhs, rhs);
}

inline bool operator<(const VlanTag& lhs, const VlanTag& rhs) {
  return lhs.value < rhs.value;
}

inline bool operator>(const VlanTag& lhs, const VlanTag& rhs) {
  return operator<(rhs, lhs);
}

inline bool operator<=(const VlanTag& lhs, const VlanTag& rhs) {
  return !operator>(lhs, rhs);
}

inline bool operator>=(const VlanTag& lhs, const VlanTag& rhs) {
  return !operator<(lhs, rhs);
}

/*
 * An Ethernet Header. The payload is not represented here.
 */
class EthHdr {
 public:
  /**
   * The length of the ethernet header:
   *  - no VLAN tag:: DA (6) + SA (6) + Protocol (2) => 14
   *  - with a single VLAN tag:: no VLAN tag + vlan (4) => 18
   *
   * When processing packets in software, if using VLAN tag, we always use a
   * single VLAN tag in our internal representation.
   */
  enum {
    UNTAGGED_PKT_SIZE = 14,
    // TODO(skhare) Fix all callsites and rename to TAGGED_PKT_SIZE
    SIZE = 18
  };
  using VlanTags_t = std::vector<VlanTag>;
  /*
   * default constructor
   */
  EthHdr() = default;
  /*
   * copy constructor
   */
  EthHdr(const EthHdr& rhs)

      = default;
  /*
   * parameterized  data constructor
   */
  EthHdr(
      const folly::MacAddress& _dstAddr,
      const folly::MacAddress& _srcAddr,
      VlanTags_t _vlanTags,
      const uint16_t _etherType)
      : dstAddr(_dstAddr),
        srcAddr(_srcAddr),
        vlanTags(std::move(_vlanTags)),
        etherType(_etherType) {}
  /*
   * cursor data constructor
   */
  explicit EthHdr(folly::io::Cursor& cursor);
  /*
   * destructor
   */
  ~EthHdr() = default;
  /*
   * operator=
   */
  EthHdr& operator=(const EthHdr& rhs) = default;
  /*
   * Convert to string
   */
  std::string toString() const;
  /*
   * Accessors
   */
  folly::MacAddress getSrcMac() const {
    return srcAddr;
  }
  folly::MacAddress getDstMac() const {
    return dstAddr;
  }
  const VlanTags_t& getVlanTags() const {
    return vlanTags;
  }
  uint16_t getEtherType() const {
    return etherType;
  }
  uint32_t size() const {
    return getVlanTags().size() == 0 ? EthHdr::UNTAGGED_PKT_SIZE : EthHdr::SIZE;
  }
  void setDstMac(const folly::MacAddress& dstMac) {
    dstAddr = dstMac;
  }
  void addVlans(
      const std::vector<VlanID>& vlans,
      ETHERTYPE ether = ETHERTYPE::ETHERTYPE_VLAN);
  void addVlan(VlanID vlan, ETHERTYPE ether = ETHERTYPE::ETHERTYPE_VLAN) {
    addVlans({vlan}, ether);
  }
  void setVlans(
      const std::vector<VlanID>& vlans,
      ETHERTYPE ether = ETHERTYPE::ETHERTYPE_VLAN) {
    vlanTags.clear();
    addVlans(vlans, ether);
  }
  void setVlan(VlanID vlan, ETHERTYPE ether = ETHERTYPE::ETHERTYPE_VLAN) {
    setVlans({vlan}, ether);
  }

  std::string printVlanTags();

 public:
  /*
   * Destination Address
   */
  folly::MacAddress dstAddr;
  /*
   * Source Address
   */
  folly::MacAddress srcAddr;
  /*
   * VLAN tags are optional
   */
  VlanTags_t vlanTags;
  /*
   * Either Ethernet II "EtherType" or IEEE 802.3 "size/length"
   */
  uint16_t etherType{0};
};

inline bool operator==(const EthHdr& lhs, const EthHdr& rhs) {
  return lhs.dstAddr == rhs.dstAddr && lhs.srcAddr == rhs.srcAddr &&
      lhs.vlanTags == rhs.vlanTags && lhs.etherType == rhs.etherType;
}

inline bool operator!=(const EthHdr& lhs, const EthHdr& rhs) {
  return !operator==(lhs, rhs);
}

// toAppend to make folly::to<string> work with EthHdr
inline void toAppend(const EthHdr& ethHdr, std::string* result) {
  result->append(ethHdr.toString());
}

inline void toAppend(const EthHdr& ethHdr, folly::fbstring* result) {
  result->append(ethHdr.toString());
}

inline std::ostream& operator<<(std::ostream& os, const EthHdr& ethHdr) {
  os << ethHdr.toString();
  return os;
}

} // namespace facebook::fboss
