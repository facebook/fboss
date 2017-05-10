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

#include <folly/Format.h>

#include <glog/logging.h>

namespace facebook {
namespace fboss {

void GlogBufferStatsLogger::logDeviceBufferStat(
    uint64_t bytesUsed,
    uint64_t bytesMax) {
  LOG(INFO) << " Switch MMU, bytes used : " << bytesUsed
            << " max available : " << bytesMax
            << " percent used : " << (bytesUsed * 100.0) / bytesMax;
}

void GlogBufferStatsLogger::logPortBufferStat(
    const std::string& portName,
    Direction dir,
    uint64_t bytesUsed) {
  LOG(INFO) << " Port : " << portName << " " << dirStr(dir)
            << " bytes used : " << bytesUsed;
}
}
}
