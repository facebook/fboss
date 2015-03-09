/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmTxPacket.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmStats.h"

using folly::IOBuf;
using std::unique_ptr;

namespace facebook { namespace fboss {

void* BcmTxPacket::allocateTxBuf(int unit, uint32_t size) {
  void* data = malloc(size);
  if (!data) {
    throw BcmError(OPENNSL_E_MEMORY, "failed to allocate ", size,
                   " bytes with soc_cm_salloc()");
  }
  return data;
}

void BcmTxPacket::freeTxBuf(void *ptr, void* arg) {
  intptr_t unit = reinterpret_cast<intptr_t>(arg);
  free(ptr);
  BcmStats::get()->txPktFree();
}

}} // facebook::fboss
