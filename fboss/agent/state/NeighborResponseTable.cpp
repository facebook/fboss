/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/NeighborResponseTable.h"

namespace {
constexpr auto kMac = "mac";
constexpr auto kIntf = "interfaceId";
} // namespace

namespace facebook::fboss {

folly::dynamic NeighborResponseEntry::toFollyDynamic() const {
  folly::dynamic entry = folly::dynamic::object;
  entry[kMac] = mac.toString();
  entry[kIntf] = static_cast<uint32_t>(interfaceID);
  return entry;
}

NeighborResponseEntry NeighborResponseEntry::fromFollyDynamic(
    const folly::dynamic& entry) {
  return NeighborResponseEntry(
      folly::MacAddress(entry[kMac].stringPiece()),
      InterfaceID(entry[kIntf].asInt()));
}

} // namespace facebook::fboss
