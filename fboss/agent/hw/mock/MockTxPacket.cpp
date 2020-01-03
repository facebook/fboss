/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/mock/MockTxPacket.h"

#include <folly/io/IOBuf.h>

#include "fboss/agent/packet/EthHdr.h"

using folly::IOBuf;

namespace facebook::fboss {

MockTxPacket::MockTxPacket(uint32_t size) {
  buf_ = IOBuf::create(size);
  buf_->append(size);
}

std::unique_ptr<MockTxPacket> MockTxPacket::clone() const {
  auto ret = std::make_unique<MockTxPacket>(buf_->capacity());
  ret->buf()->clear();
  ret->buf()->advance(EthHdr::SIZE);
  memcpy(ret->buf()->writableData(), buf_->data(), buf_->length());
  ret->buf()->append(buf_->length());

  return ret;
}

} // namespace facebook::fboss
