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

extern "C" {
#include <opennsl/types.h>
}

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>

namespace facebook { namespace fboss {

void macFromBcm(opennsl_mac_t mac, folly::MacAddress* result);
folly::MacAddress macFromBcm(const opennsl_mac_t mac);
void macToBcm(folly::MacAddress mac, opennsl_mac_t* result);

// On some dual stack BCM functions, both v4 and v6 addresses are packed into
// the _same_ opennsl_ip6_t class.
void ipToBcmIp6(const folly::IPAddress& ip, opennsl_ip6_t* bcmIp6);
void networkToBcmIp6(
  const folly::CIDRNetwork& network,
  opennsl_ip6_t* ip,
  opennsl_ip6_t* mask);

}} //faceboook::fboss
