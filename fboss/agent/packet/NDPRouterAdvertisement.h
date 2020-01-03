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
 * Neighbor Discovery Protocol
 *
 * References:
 *   https://en.wikipedia.org/wiki/Neighbor_Discovery_Protocol
 *   https://tools.ietf.org/html/rfc4861
 */
#include <folly/io/Cursor.h>
#include "fboss/agent/packet/HdrParseError.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"

namespace facebook::fboss {

/*
 * NDP Router Advertisement message.
 */
class NDPRouterAdvertisement {
 public:
  /*
   * default constructor
   */
  NDPRouterAdvertisement() {}
  /*
   * copy constructor
   */
  NDPRouterAdvertisement(const NDPRouterAdvertisement& rhs)
      : curHopLimit(rhs.curHopLimit),
        flags(rhs.flags),
        routerLifetime(rhs.routerLifetime),
        reachableTime(rhs.reachableTime),
        retransTimer(rhs.retransTimer) {}
  /*
   * parameterized data constructor
   */
  NDPRouterAdvertisement(
      uint8_t _curHopLimit,
      uint8_t _flags,
      uint16_t _routerLifetime,
      uint32_t _reachableTime,
      uint32_t _retransTimer)
      : curHopLimit(_curHopLimit),
        flags(_flags),
        routerLifetime(_routerLifetime),
        reachableTime(_reachableTime),
        retransTimer(_retransTimer) {}
  /*
   * cursor data constructor
   */
  explicit NDPRouterAdvertisement(folly::io::Cursor& cursor);
  /*
   * destructor
   */
  ~NDPRouterAdvertisement() {}
  /*
   * operator=
   */
  NDPRouterAdvertisement& operator=(const NDPRouterAdvertisement& rhs) {
    curHopLimit = rhs.curHopLimit;
    flags = rhs.flags;
    routerLifetime = rhs.routerLifetime;
    reachableTime = rhs.reachableTime;
    retransTimer = rhs.retransTimer;
    return *this;
  }
  /*
   * Validate this packet.
   */
  void validate(const IPv6Hdr& ipv6Hdr, const ICMPHdr& icmpv6Hdr);
  /*
   * Getters.
   */
  const bool managedAddressConfiguration() const {
    return M();
  }
  const bool M() const {
    return static_cast<bool>(flags & 0x80);
  }
  const bool otherConfiguration() const {
    return O();
  }
  const bool O() const {
    return static_cast<bool>(flags & 0x40);
  }
  const bool homeAgent() const {
    return static_cast<bool>(flags & 0x20);
  }
  const bool defaultRouterPreference() const {
    return prf();
  }
  const bool prf() const {
    return (flags & 0x18) >> 3;
  }
  const bool proxy() const {
    return static_cast<bool>(flags & 0x04);
  }

 public:
  /*
   * 8-bit unsigned integer. The default value that should be placed in the
   * Hop Count field of the IP header for outgoing IP packets. A value of zero
   * means unspecified (by this router).
   */
  uint8_t curHopLimit{0};
  /*
   * 1-bit "Managed address configuration" flag. When set, it indicates that
   * addresses are available via Dynamic Host Configuration Protocol [DHCPv6].
   * If the M flag is set, the O flag is redundant and can be ignored because
   * DHCPv6 will return all available configuration information.
   *
   * 1-bit "Other configuration" flag. When set, it indicates that other
   * configuration information is available via DHCPv6. Examples of such
   * information are DNS-related information or information on other servers
   * within the network.
   *
   * 1-bit Home Agent
   *
   * 2-bit prf (Default Router Preference)
   *
   * 1-bit Proxy
   *
   * 2-bit RESERVED (0)
   */
  uint8_t flags{0};
  /*
   * The lifetime associated with the default router in units of seconds. The
   * field can contain values up to 65535 and receivers should handle any
   * value, while the sending rules in Section 6 limit the lifetime to 9000
   * seconds. A Lifetime of 0 indicates that the router is not a default
   * router and SHOULD NOT appear on the default router list. The Router
   * Lifetime applies only to the router's usefulness as a default router; it
   * does not apply to information contained in other message fields or
   * options. Options that need time limits for their information include
   * their own lifetime fields.
   */
  uint16_t routerLifetime{0};
  /*
   * The time, in milliseconds, that a node assumes a neighbor is reachable
   * after having received a reachability confirmation. Used by the Neighbor
   * Unreachability Detection algorithm. A value of zero means unspecified (by
   * this router).
   */
  uint32_t reachableTime{0};
  /*
   * The time, in milliseconds, between retransmitted Neighbor Solicitation
   * messages. Used by address resolution and the Neighbor Unreachability
   * Detection algorithm. A value of zero means unspecified (by this router).
   */
  uint32_t retransTimer{0};
  // TODO: add support for options
};

inline bool operator==(
    const NDPRouterAdvertisement& lhs,
    const NDPRouterAdvertisement& rhs) {
  return lhs.curHopLimit == rhs.curHopLimit && lhs.flags == rhs.flags &&
      lhs.routerLifetime == rhs.routerLifetime &&
      lhs.reachableTime == rhs.reachableTime &&
      lhs.retransTimer == rhs.retransTimer;
}

inline bool operator!=(
    const NDPRouterAdvertisement& lhs,
    const NDPRouterAdvertisement& rhs) {
  return !operator==(lhs, rhs);
}

} // namespace facebook::fboss
