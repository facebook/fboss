/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/CounterApi.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class CounterApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    counterApi = std::make_unique<CounterApi>();
  }
  void checkCounter(
      CounterSaiId id,
      SaiCharArray32& label,
      sai_counter_type_t type) {
    EXPECT_EQ(
        type,
        counterApi->getAttribute(id, SaiCounterTraits::Attributes::Type{}));
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
    EXPECT_EQ(
        label,
        counterApi->getAttribute(id, SaiCounterTraits::Attributes::Label{}));
#endif
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<CounterApi> counterApi;
};

TEST_F(CounterApiTest, counter) {
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  SaiCharArray32 label{"testCounter"};
  auto counterId = counterApi->create<SaiCounterTraits>(
      SaiCounterTraits::CreateAttributes{label, SAI_COUNTER_TYPE_REGULAR}, 0);
  checkCounter(counterId, label, SAI_COUNTER_TYPE_REGULAR);
#endif
}
