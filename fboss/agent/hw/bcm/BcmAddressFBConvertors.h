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
#include <bcm/types.h>
}

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>

namespace facebook::fboss {

void macFromBcm(bcm_mac_t mac, folly::MacAddress* result);
folly::MacAddress macFromBcm(const bcm_mac_t mac);
void macToBcm(folly::MacAddress mac, bcm_mac_t* result);

// On some dual stack BCM functions, both v4 and v6 addresses are packed into
// the _same_ bcm_ip6_t class.
void ipToBcmIp6(const folly::IPAddress& ip, bcm_ip6_t* bcmIp6);
folly::IPAddress ipFromBcm(bcm_ip6_t bcmIp6);
void networkToBcmIp6(
    const folly::CIDRNetwork& network,
    bcm_ip6_t* ip,
    bcm_ip6_t* mask);

} // namespace facebook::fboss
