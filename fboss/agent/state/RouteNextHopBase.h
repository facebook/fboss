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

#include <folly/IPAddress.h>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"

namespace facebook { namespace fboss {

class RouteNextHopBase {
 public:
  explicit RouteNextHopBase(
      folly::IPAddress addr, folly::Optional<InterfaceID> intfID = folly::none);

  /* Utility function to get thrift representation of this nexthop */
  network::thrift::BinaryAddress toThrift() const;

  const folly::IPAddress& addr() const noexcept {
    return addr_;
  }

  const folly::Optional<InterfaceID>& intfID() const noexcept {
    return intfID_;
  }

  std::string str() const;

 private:
  folly::IPAddress addr_;
  folly::Optional<InterfaceID> intfID_;
};

/**
 * Comparision operators
 */
bool operator==(const RouteNextHopBase& a, const RouteNextHopBase& b);
bool operator< (const RouteNextHopBase& a, const RouteNextHopBase& b);

}}
