/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss::utility {

void disableTTLDecrements(
    HwSwitch* /*hw*/,
    RouterID /*routerId*/,
    InterfaceID /*intf*/,
    const folly::IPAddress& /*nhop*/) {
  throw FbossError("Disable TTL decrement not iimplemented");
}
} // namespace facebook::fboss::utility
