/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/UdfApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class UdfApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    udfApi = std::make_unique<UdfApi>();
  }

  sai_uint16_t kUdfGroupLength() {
    return 2;
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<UdfApi> udfApi;
};

TEST_F(UdfApiTest, createUdfGroup) {
  SaiUdfGroupTraits::Attributes::Type type{SAI_UDF_GROUP_TYPE_HASH};
  SaiUdfGroupTraits::Attributes::Length length{kUdfGroupLength()};
  auto udfGroupId = udfApi->create<SaiUdfGroupTraits>({type, length}, 0);

  EXPECT_EQ(udfGroupId, fs->udfGroupManager.get(udfGroupId).id);

  EXPECT_EQ(
      udfApi->getAttribute(udfGroupId, SaiUdfGroupTraits::Attributes::Type{}),
      SAI_UDF_GROUP_TYPE_HASH);
  EXPECT_EQ(
      udfApi->getAttribute(udfGroupId, SaiUdfGroupTraits::Attributes::Length{}),
      kUdfGroupLength());
}

TEST_F(UdfApiTest, removeUdfGroup) {
  SaiUdfGroupTraits::Attributes::Type type{SAI_UDF_GROUP_TYPE_HASH};
  SaiUdfGroupTraits::Attributes::Length length{kUdfGroupLength()};
  auto udfGroupId = udfApi->create<SaiUdfGroupTraits>({type, length}, 0);

  EXPECT_EQ(udfGroupId, fs->udfGroupManager.get(udfGroupId).id);
  udfApi->remove(udfGroupId);
}

TEST_F(UdfApiTest, setUdfGroupAttributes) {
  SaiUdfGroupTraits::Attributes::Type type{SAI_UDF_GROUP_TYPE_HASH};
  SaiUdfGroupTraits::Attributes::Length length{kUdfGroupLength()};
  auto udfGroupId = udfApi->create<SaiUdfGroupTraits>({type, length}, 0);

  EXPECT_EQ(udfGroupId, fs->udfGroupManager.get(udfGroupId).id);

  EXPECT_THROW(
      udfApi->setAttribute(
          udfGroupId,
          SaiUdfGroupTraits::Attributes::Type{SAI_UDF_GROUP_TYPE_GENERIC}),
      SaiApiError);
  EXPECT_THROW(
      udfApi->setAttribute(
          udfGroupId,
          SaiUdfGroupTraits::Attributes::Length{kUdfGroupLength() + 1}),
      SaiApiError);
}
