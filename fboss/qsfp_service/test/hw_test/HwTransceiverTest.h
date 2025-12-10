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

#include "fboss/qsfp_service/test/hw_test/HwTest.h"

namespace facebook::fboss {

class HwTransceiverTest : public HwTest {
 public:
  explicit HwTransceiverTest(bool isPortUp) : isPortUp_(isPortUp) {}

  void SetUp() override;

  const std::vector<TransceiverID>& getExpectedTransceivers() const {
    return expectedTcvrs_;
  }

  std::unique_ptr<std::vector<int32_t>> getExpectedLegacyTransceiverIds() const;

 protected:
  bool isPortUp_{false};

 private:
  std::vector<TransceiverID> expectedTcvrs_;
  // Whether needs to bring up the port on the agent side.
};
} // namespace facebook::fboss
