/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmBstStatsMgr.h"
#include "fboss/agent/hw/BufferStatsLogger.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook {
namespace fboss {

// stubbed out
BcmBstStatsMgr::BcmBstStatsMgr(BcmSwitch* /*hw*/) {}
bool BcmBstStatsMgr::startBufferStatCollection() {
  return false;
}
bool BcmBstStatsMgr::stopBufferStatCollection() {
  return true;
}
void BcmBstStatsMgr::updateStats() {}
} // namespace fboss
} // namespace facebook
