/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/platforms/wedge/tests/MockWedgeManager.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/tests/MockTransceiverImpl.h"

#include <folly/Memory.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace facebook::fboss;
using namespace ::testing;
namespace {

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

TEST_F(WedgeManagerTest, readTransceiver) {
  std::map<int32_t, ReadResponse> response;
  std::unique_ptr<ReadRequest> request(new ReadRequest);
  TransceiverIOParameters param;
  std::vector<int32_t> data = {1, 3, 7};
  request->ids_ref() = data;
  param.offset_ref() = 0x10;
  param.length_ref() = 10;
  request->parameter_ref() = param;

  wedgeManager_->readTransceiverRegister(response, std::move(request));
  for (const auto& i : data) {
    EXPECT_NE(response.find(i), response.end());
  }
}

TEST_F(WedgeManagerTest, writeTransceiver) {
  std::map<int32_t, WriteResponse> response;
  std::unique_ptr<WriteRequest> request(new WriteRequest);
  TransceiverIOParameters param;
  std::vector<int32_t> data = {1, 3, 7};
  request->ids_ref() = data;
  param.offset_ref() = 0x10;
  request->parameter_ref() = param;
  request->data_ref() = 0xab;

  wedgeManager_->writeTransceiverRegister(response, std::move(request));
  for (const auto& i : data) {
    EXPECT_NE(response.find(i), response.end());
  }
}
}
