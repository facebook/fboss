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

#include "fboss/agent/state/RouteNextHop.h"

namespace facebook { namespace fboss {

class BcmHostKey : public RouteNextHop {

 public:
  // Constructor based on the forward info
  BcmHostKey(opennsl_vrf_t vrf, const RouteNextHop& fwd)
      : BcmHostKey(vrf, fwd.addr(), fwd.intfID()) {
  }

  // Constructor based on the IP address
  BcmHostKey(
      opennsl_vrf_t vrf,
      folly::IPAddress addr,
      folly::Optional<InterfaceID> intfID = folly::none);

  opennsl_vrf_t getVrf() const {
    return vrf_;
  }

  std::string str() const;

 private:
  opennsl_vrf_t vrf_;
};

/**
 * Comparision operators
 */
bool operator==(const BcmHostKey& a, const BcmHostKey& b);
bool operator< (const BcmHostKey& a, const BcmHostKey& b);

void toAppend(const BcmHostKey& key, std::string *result);
std::ostream& operator<<(std::ostream& os, const BcmHostKey& key);

}}
