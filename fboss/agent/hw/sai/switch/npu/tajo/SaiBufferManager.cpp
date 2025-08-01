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

#ifdef IS_OSS
void SaiBufferManager::publishGlobalWatermarks(
    const uint64_t& /*globalHeadroomBytes*/,
    const uint64_t& /*globalSharedBytes*/) const {};

void SaiBufferManager::publishPgWatermarks(
    const std::string& /*portName*/,
    const int& /*pg*/,
    const uint64_t& /*pgHeadroomBytes*/,
    const uint64_t& /*pgSharedBytes*/) const {};
#endif

void SaiBufferManager::loadCpuPortEgressBufferPool() {}
} // namespace facebook::fboss
