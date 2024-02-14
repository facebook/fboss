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
  void checkInCounter(DebugCounterSaiId id, size_t inSize) {
    EXPECT_EQ(
        SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
        debugCounterApi->getAttribute(
            id, SaiInPortDebugCounterTraits::Attributes::Type{}));
    EXPECT_EQ(
        SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
        debugCounterApi->getAttribute(
            id, SaiInPortDebugCounterTraits::Attributes::BindMethod{}));
    auto inDropReasons = debugCounterApi->getAttribute(
        id, SaiInPortDebugCounterTraits::Attributes::DropReasons{});
    EXPECT_EQ(inDropReasons.size(), inSize);
  }
  void checkOutCounter(DebugCounterSaiId id, size_t outSize) {
    EXPECT_EQ(
        SAI_DEBUG_COUNTER_TYPE_PORT_OUT_DROP_REASONS,
        debugCounterApi->getAttribute(
            id, SaiOutPortDebugCounterTraits::Attributes::Type{}));
    EXPECT_EQ(
        SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
        debugCounterApi->getAttribute(
            id, SaiOutPortDebugCounterTraits::Attributes::BindMethod{}));
    auto outDropReasons = debugCounterApi->getAttribute(
        id, SaiOutPortDebugCounterTraits::Attributes::DropReasons{});
    EXPECT_EQ(outDropReasons.size(), outSize);
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<DebugCounterApi> debugCounterApi;
};

TEST_F(DebugCounterApiTest, portInDebugCounterEmptyReasons) {
  auto debugCounterId = debugCounterApi->create<SaiInPortDebugCounterTraits>(
      SaiInPortDebugCounterTraits::CreateAttributes{
          SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
          SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
          std::optional<
              SaiInPortDebugCounterTraits::Attributes::DropReasons>{}},
      0);
  checkInCounter(debugCounterId, 0);
}

TEST_F(DebugCounterApiTest, portInDebugCounter) {
  auto debugCounterId = debugCounterApi->create<SaiInPortDebugCounterTraits>(
      SaiInPortDebugCounterTraits::CreateAttributes{
          SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
          SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
          SaiInPortDebugCounterTraits::Attributes::DropReasons{
              {SAI_IN_DROP_REASON_BLACKHOLE_ROUTE}}},
      0);
  checkInCounter(debugCounterId, 1);
}

TEST_F(DebugCounterApiTest, portOutDebugCounter) {
  auto debugCounterId = debugCounterApi->create<SaiOutPortDebugCounterTraits>(
      SaiOutPortDebugCounterTraits::CreateAttributes{
          SAI_DEBUG_COUNTER_TYPE_PORT_OUT_DROP_REASONS,
          SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
          SaiOutPortDebugCounterTraits::Attributes::DropReasons{
              {SAI_OUT_DROP_REASON_L3_ANY}}},
      0);
  checkOutCounter(debugCounterId, 1);
}
