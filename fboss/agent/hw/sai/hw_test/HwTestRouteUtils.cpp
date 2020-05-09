/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestRouteUtils.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::utility {

// TODO(skhare) implement this method for SAI
std::optional<cfg::AclLookupClass> getHwRouteClassID(
    const HwSwitch* /*unused*/,
    RouterID /*unused*/,
    const folly::CIDRNetwork& /*unused*/) {
  throw FbossError("Not implemented");
  return std::nullopt;
}

} // namespace facebook::fboss::utility
