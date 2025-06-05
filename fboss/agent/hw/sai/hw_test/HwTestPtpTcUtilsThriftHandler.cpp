/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook::fboss::utility {
bool HwTestThriftHandler::getPtpTcEnabled() {
  auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch_);
  XLOG(DBG2) << "getPtpTcEnabled: "
             << saiSwitch->managerTable()->switchManager().isPtpTcEnabled();
  return saiSwitch->managerTable()->switchManager().isPtpTcEnabled();
}
} // namespace facebook::fboss::utility
