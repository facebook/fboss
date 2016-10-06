/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#include "SaiTxPacket.h"
#include "SaiError.h"


namespace facebook { namespace fboss {

SaiTxPacket::SaiTxPacket(uint32_t size) {

  if (size == 0) {
    throw SaiError("Could not allocate SaiTxPacket with zero buffer size.");
  }

  buf_ = folly::IOBuf::takeOwnership(new uint8_t[size], // void* buf
                                     size,              // uint32_t capacity
                                     freeTxBufCallback, // Free Function freeFn
                                     NULL);             // void* userData
}

SaiTxPacket::~SaiTxPacket() {
  // Nothing to do.  The IOBuf destructor will call freeTxBufCallback()
  // to free the packet data
}

void SaiTxPacket::freeTxBufCallback(void *ptr, void* arg) {
  delete[] static_cast<uint8_t*>(ptr);
}

}} // facebook::fboss
