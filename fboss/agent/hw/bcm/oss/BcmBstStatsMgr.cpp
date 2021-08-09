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

namespace facebook::fboss {

BcmBstStatsMgr::BcmBstStatsMgr(BcmSwitch* hw)
    : hw_(hw), bufferStatsLogger_(std::make_unique<GlogBufferStatsLogger>()) {}

void BcmBstStatsMgr::publishQueueuWatermark(
    const std::string& /*portName*/,
    int /*queue*/,
    uint64_t /*peakBytes*/) const {}
void BcmBstStatsMgr::publishPgWatermarks(
    const std::string& /*portName*/,
    const uint64_t& /*pgHeadroomBytes*/,
    const uint64_t& /*pgSharedBytes*/) const {}
void BcmBstStatsMgr::publishGlobalWatermarks(
    const int /*itm*/,
    const uint64_t& /*globalHeadroomBytes*/,
    const uint64_t& /*globalSharedBytes*/) const {}

void BcmBstStatsMgr::publishDeviceWatermark(uint64_t /*peakBytes*/) const {}
void BcmBstStatsMgr::publishCpuQueueWatermark(
    int /*queue*/,
    uint64_t /*peakBytes*/) const {}
} // namespace facebook::fboss
