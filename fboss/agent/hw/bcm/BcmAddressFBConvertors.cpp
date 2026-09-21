/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"

#include <folly/Range.h>

namespace facebook::fboss {

void macFromBcm(bcm_mac_t mac, folly::MacAddress* result) {
  result->setFromBinary(folly::ByteRange(mac, folly::MacAddress::SIZE));
}
folly::MacAddress macFromBcm(const bcm_mac_t mac) {
  return folly::MacAddress::fromBinary(
      folly::ByteRange(mac, folly::MacAddress::SIZE));
}
void macToBcm(folly::MacAddress mac, bcm_mac_t* result) {
  memcpy(result, mac.bytes(), folly::MacAddress::SIZE);
}

// On some dual stack BCM functions, both v4 and v6 addresses are packed into
// the _same_ bcm_ip6_t class.
void ipToBcmIp6(const folly::IPAddress& ip, bcm_ip6_t* bcmIp6) {
  memset(bcmIp6, 0, sizeof(bcm_ip6_t));
  if (ip.isV6()) {
    memcpy(bcmIp6, ip.bytes(), ip.byteCount());
  } else {
    memcpy(((uint8_t*)bcmIp6) + 16 - 4, ip.bytes(), ip.byteCount());
  }
}

folly::IPAddress ipFromBcm(bcm_ip6_t bcmIp6) {
  return folly::IPAddress::fromBinary(folly::ByteRange(bcmIp6, 16));
}

void networkToBcmIp6(
    const folly::CIDRNetwork& network,
    bcm_ip6_t* ip,
    bcm_ip6_t* mask) {
  ipToBcmIp6(network.first, ip);
  if (network.first.isV4()) {
    ipToBcmIp6(
        folly::IPAddressV4(folly::IPAddressV4::fetchMask(network.second)),
        mask);
  } else {
    ipToBcmIp6(
        folly::IPAddressV6(folly::IPAddressV6::fetchMask(network.second)),
        mask);
  }
}

} // namespace facebook::fboss
