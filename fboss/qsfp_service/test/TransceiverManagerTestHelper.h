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

#include "fboss/qsfp_service/platforms/wedge/tests/MockWedgeManager.h"

#include <folly/experimental/TestUtil.h>
#include <gtest/gtest.h>

namespace facebook::fboss {

class TransceiverManagerTestHelper : public ::testing::Test {
 public:
  static constexpr int numModules = 16;

  void SetUp() override;

  void resetTransceiverManager();

  folly::test::TemporaryDirectory tmpDir = folly::test::TemporaryDirectory();
  std::string qsfpSvcVolatileDir = tmpDir.path().string();
  std::string agentCfgPath = qsfpSvcVolatileDir + "/fakeAgentConfig";
  std::string qsfpCfgPath = qsfpSvcVolatileDir + "/fakeQsfpConfig";

  std::unique_ptr<MockWedgeManager> transceiverManager_;
};

} // namespace facebook::fboss
