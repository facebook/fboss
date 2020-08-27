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
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<DebugCounterApi> debugCounterApi;
};

TEST_F(DebugCounterApiTest, portInDebugCounterEmptyReasons) {
  auto debugCounterid = debugCounterApi->create<SaiDebugCounterTraits>(
      SaiDebugCounterTraits::CreateAttributes{
          SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
          SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
          std::optional<SaiDebugCounterTraits::Attributes::InDropReasons>{},
          std::nullopt},
      0);
  auto inDropReasons = debugCounterApi->getAttribute(
      debugCounterid, SaiDebugCounterTraits::Attributes::InDropReasons{});
  auto outDropReasons = debugCounterApi->getAttribute(
      debugCounterid, SaiDebugCounterTraits::Attributes::OutDropReasons{});
  EXPECT_EQ(inDropReasons.size(), 0);
  EXPECT_EQ(outDropReasons.size(), 0);
}

TEST_F(DebugCounterApiTest, portOutDebugCounterEmptyReasons) {
  auto debugCounterid = debugCounterApi->create<SaiDebugCounterTraits>(
      SaiDebugCounterTraits::CreateAttributes{
          SAI_DEBUG_COUNTER_TYPE_PORT_OUT_DROP_REASONS,
          SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
          std::nullopt,
          std::optional<SaiDebugCounterTraits::Attributes::OutDropReasons>{}},
      0);
  auto inDropReasons = debugCounterApi->getAttribute(
      debugCounterid, SaiDebugCounterTraits::Attributes::InDropReasons{});
  auto outDropReasons = debugCounterApi->getAttribute(
      debugCounterid, SaiDebugCounterTraits::Attributes::OutDropReasons{});
  EXPECT_EQ(inDropReasons.size(), 0);
  EXPECT_EQ(outDropReasons.size(), 0);
}
