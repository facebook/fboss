/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiBufferManager.h"

namespace facebook::fboss {

void SaiBufferManager::publishDeviceWatermark(uint64_t /*peakBytes*/) const {}
void SaiBufferManager::publishGlobalWatermarks(
    const uint64_t& /*globalHeadroomBytes*/,
    const uint64_t& /*globalSharedBytes*/) const {};
} // namespace facebook::fboss
