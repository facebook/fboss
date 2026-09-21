/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/types.h"

namespace facebook::fboss {

namespace cfg {

std::ostream& operator<<(std::ostream& out, LoadBalancerID id) {
  std::string idAsString;

  switch (id) {
    case LoadBalancerID::AGGREGATE_PORT:
      idAsString = "AggregatePort";
      break;
    case LoadBalancerID::ECMP:
      idAsString = "ECMP";
      break;
  }

  return out << idAsString;
}

} // namespace cfg

} // namespace facebook::fboss
