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

namespace facebook::fboss {

class MockTxPacket : public TxPacket {
 public:
  explicit MockTxPacket(uint32_t size);

  std::unique_ptr<MockTxPacket> clone() const;
};

} // namespace facebook::fboss
