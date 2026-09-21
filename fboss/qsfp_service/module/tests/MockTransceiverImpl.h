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

#include "fboss/qsfp_service/module/TransceiverImpl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace facebook {
namespace fboss {

class MockTransceiverImpl : public TransceiverImpl {
 public:
  MOCK_METHOD3(
      readTransceiver,
      int(const TransceiverAccessParameter&, uint8_t*, const int));
  MOCK_METHOD4(
      writeTransceiver,
      int(const TransceiverAccessParameter&,
          const uint8_t*,
          uint64_t,
          const int));
  MOCK_METHOD0(detectTransceiver, bool());
  MOCK_METHOD0(getName, folly::StringPiece());
  MOCK_CONST_METHOD0(getNum, int());
  MOCK_METHOD0(triggerQsfpHardReset, void());
};
} // namespace fboss
} // namespace facebook
