/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/phy/SaiPhyRetimer.h"

#include <folly/Format.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::phy {

bool SaiPhyRetimer::isSupported(Feature feature) const {
  switch (feature) {
    case Feature::PRBS:
    case Feature::PRBS_STATS:
    case Feature::LOOPBACK:
    case Feature::PORT_STATS:
    case Feature::PORT_INFO:
    case Feature::MACSEC:
      return false;
    default:
      return true;
  }
}

} // namespace facebook::fboss::phy
