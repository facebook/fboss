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
#pragma once

#include "fboss/agent/TxPacket.h"


namespace facebook { namespace fboss {

class SaiTxPacket : public TxPacket {
public:
  /*
   * Creates a SaiTxPacket with buffer of size 'size'.   
   */
  SaiTxPacket(uint32_t size);

  virtual ~SaiTxPacket();

private:
  /*
   * IOBuf callback which releases memory allocated for packet data. 
   */
  static void freeTxBufCallback(void *ptr, void* arg);
};

}} // facebook::fboss
