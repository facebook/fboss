/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmPtpTcMgr.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook::fboss::utility {

bool HwTestThriftHandler::getPtpTcEnabled() {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);
  XLOG(DBG2) << "getPtpTcEnabled: " << BcmPtpTcMgr::isPtpTcEnabled(bcmSwitch);

  return BcmPtpTcMgr::isPtpTcEnabled(bcmSwitch);
}

} // namespace facebook::fboss::utility
