/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/DebugCounterApi.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class DebugCounterApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    debugCounterApi = std::make_unique<DebugCounterApi>();
  }
  void checkCounter(
      DebugCounterSaiId id,
      sai_debug_counter_type_t type,
      size_t inSize) {
    EXPECT_EQ(
        type,
        debugCounterApi->getAttribute(
            id, SaiDebugCounterTraits::Attributes::Type{}));
    EXPECT_EQ(
        SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
        debugCounterApi->getAttribute(
            id, SaiDebugCounterTraits::Attributes::BindMethod{}));
    auto inDropReasons = debugCounterApi->getAttribute(
        id, SaiDebugCounterTraits::Attributes::InDropReasons{});
    EXPECT_EQ(inDropReasons.size(), inSize);
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<DebugCounterApi> debugCounterApi;
};

TEST_F(DebugCounterApiTest, portInDebugCounterEmptyReasons) {
  auto debugCounterId = debugCounterApi->create<SaiDebugCounterTraits>(
      SaiDebugCounterTraits::CreateAttributes{
          SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
          SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
          std::optional<SaiDebugCounterTraits::Attributes::InDropReasons>{}},
      0);
  checkCounter(debugCounterId, SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS, 0);
}

TEST_F(DebugCounterApiTest, portInDebugCounter) {
  auto debugCounterId = debugCounterApi->create<SaiDebugCounterTraits>(
      SaiDebugCounterTraits::CreateAttributes{
          SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
          SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
          SaiDebugCounterTraits::Attributes::InDropReasons{
              {SAI_IN_DROP_REASON_BLACKHOLE_ROUTE}}},
      0);
  checkCounter(debugCounterId, SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS, 1);
}
