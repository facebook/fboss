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

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/NeighborCache.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>
#include <folly/IPAddressV6.h>

namespace facebook { namespace fboss {

enum ICMPv6Type : uint8_t;

class NdpCache : public NeighborCache<NdpTable> {
 public:
  NdpCache(SwSwitch* sw, const SwitchState* state,
           VlanID vlanID, InterfaceID intfID);

  void sentNeighborSolicitation(folly::IPAddressV6 ip);
  void receivedNdpMine(folly::IPAddressV6 ip,
                       folly::MacAddress mac,
                       PortID port,
                       ICMPv6Type type);
  void receivedNdpNotMine(folly::IPAddressV6 ip,
                          folly::MacAddress mac,
                          PortID port,
                          ICMPv6Type type);

  void probeFor(folly::IPAddressV6 ip) const override;

};

}} // facebook::fboss
