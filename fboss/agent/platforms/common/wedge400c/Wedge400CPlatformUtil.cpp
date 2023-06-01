/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/common/wedge400c/Wedge400CPlatformUtil.h"
#include <folly/logging/xlog.h>
#include "configerator/structs/neteng/netwhoami/gen-cpp2/netwhoami_types.h"
#include "neteng/netwhoami/lib/cpp/Recover.h"

namespace facebook::fboss::utility {
bool isRackTypeGrandTeton(auto rackType) {
  if (rackType &&
      ((*rackType == facebook::netwhoami::RackType::GRAND_TETON) ||
       (*rackType == facebook::netwhoami::RackType::GRAND_TETON_INFERENCE) ||
       (*rackType == facebook::netwhoami::RackType::GENOA_INFERENCE))) {
    return true;
  }

  return false;
}

bool isWedge400CPlatformRackTypeGrandTeton() {
  try {
    auto whoami = facebook::netwhoami::recoverWhoAmI();
    if (auto rackType = whoami.rack_type()) {
      auto role = whoami.role();
      XLOG(DBG2) << "isWedge400CPlatformRackTypeGrandTeton rackType: "
                 << (int)(*rackType) << "role: " << (int)(*role);
      // Checking both role and rack type to determine which
      // wedge 400c platform mapping to pick
      if (isRackTypeGrandTeton(rackType) &&
          (*role == facebook::netwhoami::Role::RSW)) {
        return true;
      }
    }
  } catch (const std::exception& ex) {
    // we won't be able to run netwhoami on netcastle switches causing
    // test failures. Switch to default behavior of no acadia rack when
    // the call fails
    XLOG(WARN)
        << "Error running netwhoami on the switch. Switch to default behavior.";
  }
  return false;
}

} // namespace facebook::fboss::utility
