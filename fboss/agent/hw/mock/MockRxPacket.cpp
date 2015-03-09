/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/mock/MockRxPacket.h"

#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/packet/PktUtil.h"

#include <locale>

using folly::IOBuf;
using folly::io::Appender;
using folly::make_unique;
using folly::StringPiece;
using std::unique_ptr;

namespace facebook { namespace fboss {

MockRxPacket::MockRxPacket(std::unique_ptr<folly::IOBuf> buf) {
  buf_ = std::move(buf);
  len_ = buf_->computeChainDataLength();
}

unique_ptr<MockRxPacket> MockRxPacket::fromHex(StringPiece hex) {
  auto buf = make_unique<IOBuf>(PktUtil::parseHexData(hex));
  return make_unique<MockRxPacket>(std::move(buf));
}

std::unique_ptr<MockRxPacket> MockRxPacket::clone() const {
  auto ret = make_unique<MockRxPacket>(buf_->clone());
  ret->srcPort_ = srcPort_;
  ret->srcVlan_ = srcVlan_;
  ret->len_ = len_;
  return ret;
}

void MockRxPacket::padToLength(uint32_t size, uint8_t pad) {
  if (len_ >= size) {
    return;
  }

  size_t padLen = size - len_;
  Appender app(buf_.get(), 1024);
  app.ensure(padLen);
  memset(app.writableData(), pad, padLen);
  app.append(padLen);
  len_ += padLen;
}

}} // facebook::fboss
