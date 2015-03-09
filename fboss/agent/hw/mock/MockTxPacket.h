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

#include "fboss/agent/TxPacket.h"

namespace facebook { namespace fboss {

class MockTxPacket : public TxPacket {
 public:
  explicit MockTxPacket(uint32_t size);

 private:
  // Forbidden copy constructor and assignment operator
  MockTxPacket(MockTxPacket const &) = delete;
  MockTxPacket& operator=(MockTxPacket const &) = delete;
};

}} // facebook::fboss
