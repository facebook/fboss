/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiTxPacket.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"

#include <folly/io/IOBuf.h>

namespace facebook::fboss {

SaiTxPacket::SaiTxPacket(uint32_t size) {
  buf_ = folly::IOBuf::createSeparate(size);
  buf_->append(size);
}

} // namespace facebook::fboss
