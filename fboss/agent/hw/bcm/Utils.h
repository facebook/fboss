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
#include <string>

extern "C" {
#include <opennsl/types.h>
}

#include <folly/MacAddress.h>

namespace facebook { namespace fboss {

inline void macFromBcm(opennsl_mac_t mac, folly::MacAddress* result) {
  result->setFromBinary(folly::ByteRange(mac, folly::MacAddress::SIZE));
}
inline folly::MacAddress macFromBcm(const opennsl_mac_t mac) {
  return folly::MacAddress::fromBinary(
      folly::ByteRange(mac, folly::MacAddress::SIZE));
}
inline void macToBcm(folly::MacAddress mac, opennsl_mac_t* result) {
  memcpy(result, mac.bytes(), folly::MacAddress::SIZE);
}

}} //faceboook::fboss
