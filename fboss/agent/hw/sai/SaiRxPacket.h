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

#include "fboss/agent/RxPacket.h"

extern "C" {
#include "saitypes.h"
}

namespace facebook { namespace fboss {

class SaiSwitch;

class SaiRxPacket : public RxPacket {
public:
  /*
   * Creates a SaiRxPacket from a sai buffer received by an rx callback.   
   */
  explicit SaiRxPacket(const void* buf,
                       sai_size_t buf_size,
                       uint32_t attr_count,
                       const sai_attribute_t *attr_list,
                       const SaiSwitch *hw);

  virtual ~SaiRxPacket();

private:
  /*
   * IOBuf callback which releases memory allocated for packet data. 
   */
  static void freeRxBufCallback(void *ptr, void* arg);
};

}} // facebook::fboss
