/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/tests/MockSffModule.h"
#include "fboss/qsfp_service/module/tests/MockTransceiverImpl.h"

#include <folly/Memory.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace facebook::fboss;
using namespace ::testing;
namespace {

class MockWedgeManager : public WedgeManager {
 public:
  explicit MockWedgeManager() : WedgeManager(nullptr, nullptr) {}
  void makeTransceiverMap() {
    for (int idx = 0; idx < getNumQsfpModules(); idx++) {
      std::unique_ptr<MockSffModule> qsfp =
          std::make_unique<MockSffModule>(
              this, nullptr, numPortsPerTransceiver());
      mockTransceivers_.emplace(TransceiverID(idx), qsfp.get());
      transceivers_.wlock()->emplace(TransceiverID(idx), move(qsfp));
    }
  }

  std::map<TransceiverID, MockSffModule*> mockTransceivers_;
};

class WedgeManagerTest : public ::testing::Test {
 public:
  void SetUp() override {
    wedgeManager_ = std::make_unique<NiceMock<MockWedgeManager>>();
    wedgeManager_->makeTransceiverMap();
  }
  std::unique_ptr<NiceMock<MockWedgeManager>> wedgeManager_;
};

TEST_F(WedgeManagerTest, getTransceiverInfo) {
  // If no ids are passed in, info for all should be returned
  for (const auto& trans : wedgeManager_->mockTransceivers_) {
    EXPECT_CALL(*trans.second, getTransceiverInfo()).Times(1);
  }
  std::map<int32_t, TransceiverInfo> transInfo;
  wedgeManager_->getTransceiversInfo(transInfo,
      std::make_unique<std::vector<int32_t>>());

  // Otherwise, just return the ids requested
  std::vector<int32_t> data = {1, 3, 7};
  for (const auto& i : data) {
    MockSffModule* qsfp = wedgeManager_->mockTransceivers_[TransceiverID(i)];
    EXPECT_CALL(*qsfp, getTransceiverInfo()).Times(1);
  }
  wedgeManager_->getTransceiversInfo(transInfo,
      std::make_unique<std::vector<int32_t>>(data));
}

}
