/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"

#include "fboss/agent/TxPacket.h"

namespace facebook::fboss {

class SaiTxPacket : public TxPacket {
 public:
  explicit SaiTxPacket(uint32_t size) {
    buf_ = folly::IOBuf::createCombined(size);
    buf_->append(size);
  }
};

} // namespace facebook::fboss
