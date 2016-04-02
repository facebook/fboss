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
#include <list>
#include <string>

namespace facebook { namespace fboss {

enum ICMPv6Type : uint8_t;

class NdpCache : public NeighborCache<NdpTable> {
 public:
  NdpCache(SwSwitch* sw, const SwitchState* state,
           VlanID vlanID, std::string vlanName, InterfaceID intfID);

  void sentNeighborSolicitation(folly::IPAddressV6 ip);
  void receivedNdpMine(folly::IPAddressV6 ip,
                       folly::MacAddress mac,
                       PortID port,
                       ICMPv6Type type,
                       uint32_t flags);
  void receivedNdpNotMine(folly::IPAddressV6 ip,
                          folly::MacAddress mac,
                          PortID port,
                          ICMPv6Type type,
                          uint32_t flags);

  void probeFor(folly::IPAddressV6 ip) const override;

  std::list<NdpEntryThrift> getNdpCacheData();

};

}} // facebook::fboss
