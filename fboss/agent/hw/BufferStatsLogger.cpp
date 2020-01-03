/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/BufferStatsLogger.h"

#include <folly/logging/xlog.h>
#include <glog/logging.h>

using std::string;

namespace facebook::fboss {

void GlogBufferStatsLogger::logDeviceBufferStat(
    uint64_t bytesUsed,
    uint64_t bytesMax) {
  XLOG(INFO) << " Switch MMU, bytes used : " << bytesUsed
             << " max available : " << bytesMax
             << " percent used : " << (bytesUsed * 100.0) / bytesMax;
}

void GlogBufferStatsLogger::logPortBufferStat(
    const std::string& portName,
    Direction dir,
    unsigned int cosQ,
    uint64_t bytesUsed,
    uint64_t pktsDropeed,
    const XPEs& xpes) {
  XLOG(INFO) << " Port : " << portName << " " << dirStr(dir)
             << " cosQ: " << cosQ << " bytes used : " << bytesUsed
             << " Packets dropped: " << pktsDropeed
             << " XPEs: " << xpeStr(xpes);
}

} // namespace facebook::fboss
