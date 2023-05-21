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

  UdfGroupSaiId createUdfGroup() {
    SaiUdfGroupTraits::Attributes::Type type{SAI_UDF_GROUP_TYPE_HASH};
    SaiUdfGroupTraits::Attributes::Length length{kUdfGroupLength()};
    return udfApi->create<SaiUdfGroupTraits>({type, length}, 0);
  }

  UdfMatchSaiId createUdfMatch() {
    SaiUdfMatchTraits::Attributes::L2Type l2Type{AclEntryFieldU16(kL2Type())};
    SaiUdfMatchTraits::Attributes::L3Type l3Type{AclEntryFieldU8(kL3Type())};
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
    SaiUdfMatchTraits::Attributes::L4DstPortType l4DstPortType{
        AclEntryFieldU16(kL4DstPort())};
    return udfApi->create<SaiUdfMatchTraits>(
        {l2Type, l3Type, l4DstPortType}, 0);
#else
    return udfApi->create<SaiUdfMatchTraits>({l2Type, l3Type}, 0);
#endif
  }

  UdfSaiId createUdf(UdfMatchSaiId udfMatchId, UdfGroupSaiId udfGroupId) {
    SaiUdfTraits::Attributes::UdfMatchId udfMatchIdAttr{udfMatchId};
    SaiUdfTraits::Attributes::UdfGroupId udfGroupIdAttr{udfGroupId};
    SaiUdfTraits::Attributes::Base baseAttr{SAI_UDF_BASE_L2};
    SaiUdfTraits::Attributes::Offset offsetAttr{kUdfOffset()};
    return udfApi->create<SaiUdfTraits>(
        {udfMatchIdAttr, udfGroupIdAttr, baseAttr, offsetAttr}, 0);
  }

  sai_uint16_t kUdfGroupLength() {
    return 2;
  }

  sai_uint16_t kUdfOffset() {
    return 2;
  }

  std::pair<sai_uint16_t, sai_uint16_t> kL2Type() const {
    return std::make_pair(0x800, 0xFFFF);
  }

  std::pair<sai_uint16_t, sai_uint16_t> kL2Type2() const {
    return std::make_pair(0x801, 0xFFFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kL3Type() const {
    return std::make_pair(0x11, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kL3Type2() const {
    return std::make_pair(0x12, 0xFF);
  }

  std::pair<sai_uint16_t, sai_uint16_t> kL4DstPort() const {
    return std::make_pair(9002, 0xFFFF);
  }

  std::pair<sai_uint16_t, sai_uint16_t> kL4DstPort2() const {
    return std::make_pair(10002, 0xFFFF);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<UdfApi> udfApi;
};

TEST_F(UdfApiTest, createUdfGroup) {
  auto udfGroupId = createUdfGroup();

  EXPECT_EQ(udfGroupId, fs->udfGroupManager.get(udfGroupId).id);

  EXPECT_EQ(
      udfApi->getAttribute(udfGroupId, SaiUdfGroupTraits::Attributes::Type{}),
      SAI_UDF_GROUP_TYPE_HASH);
  EXPECT_EQ(
      udfApi->getAttribute(udfGroupId, SaiUdfGroupTraits::Attributes::Length{}),
      kUdfGroupLength());
}

TEST_F(UdfApiTest, removeUdfGroup) {
  auto udfGroupId = createUdfGroup();

  EXPECT_EQ(udfGroupId, fs->udfGroupManager.get(udfGroupId).id);
  udfApi->remove(udfGroupId);
}

TEST_F(UdfApiTest, setUdfGroupAttributes) {
  auto udfGroupId = createUdfGroup();

  EXPECT_EQ(udfGroupId, fs->udfGroupManager.get(udfGroupId).id);

  EXPECT_THROW(
      udfApi->setAttribute(
          udfGroupId,
          SaiUdfGroupTraits::Attributes::Type{SAI_UDF_GROUP_TYPE_GENERIC}),
      SaiApiError);
  EXPECT_THROW(
      udfApi->setAttribute(
          udfGroupId,
          SaiUdfGroupTraits::Attributes::Length{
              (uint16_t)(kUdfGroupLength() + 1)}),
      SaiApiError);
}

TEST_F(UdfApiTest, createUdfMatch) {
  auto udfMatchId = createUdfMatch();

  EXPECT_EQ(udfMatchId, fs->udfMatchManager.get(udfMatchId).id);

  EXPECT_EQ(
      udfApi->getAttribute(udfMatchId, SaiUdfMatchTraits::Attributes::L2Type{}),
      AclEntryFieldU16(kL2Type()));
  EXPECT_EQ(
      udfApi->getAttribute(udfMatchId, SaiUdfMatchTraits::Attributes::L3Type{}),
      AclEntryFieldU8(kL3Type()));
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  EXPECT_EQ(
      udfApi->getAttribute(
          udfMatchId, SaiUdfMatchTraits::Attributes::L4DstPortType{}),
      AclEntryFieldU16(kL4DstPort()));
#endif
}

TEST_F(UdfApiTest, removeUdfMatch) {
  auto udfMatchId = createUdfMatch();

  EXPECT_EQ(udfMatchId, fs->udfMatchManager.get(udfMatchId).id);
  udfApi->remove(udfMatchId);
}

TEST_F(UdfApiTest, setUdfMatch) {
  auto udfMatchId = createUdfMatch();

  EXPECT_EQ(udfMatchId, fs->udfMatchManager.get(udfMatchId).id);

  EXPECT_THROW(
      udfApi->setAttribute(
          udfMatchId,
          SaiUdfMatchTraits::Attributes::L2Type{AclEntryFieldU16(kL2Type2())}),
      SaiApiError);
  EXPECT_THROW(
      udfApi->setAttribute(
          udfMatchId,
          SaiUdfMatchTraits::Attributes::L3Type{AclEntryFieldU8(kL3Type2())}),
      SaiApiError);
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  EXPECT_THROW(
      udfApi->setAttribute(
          udfMatchId,
          SaiUdfMatchTraits::Attributes::L4DstPortType{
              AclEntryFieldU16(kL4DstPort2())}),
      SaiApiError);
#endif
}

TEST_F(UdfApiTest, createUdf) {
  auto udfMatchId = createUdfMatch();
  auto udfGroupId = createUdfGroup();
  auto udfId = createUdf(udfMatchId, udfGroupId);

  EXPECT_EQ(udfId, fs->udfMatchManager.get(udfId).id);

  EXPECT_EQ(
      udfApi->getAttribute(udfId, SaiUdfTraits::Attributes::UdfMatchId{}),
      udfMatchId);
  EXPECT_EQ(
      udfApi->getAttribute(udfId, SaiUdfTraits::Attributes::UdfGroupId{}),
      udfGroupId);
  EXPECT_EQ(
      udfApi->getAttribute(udfId, SaiUdfTraits::Attributes::Base{}),
      SAI_UDF_BASE_L2);
  EXPECT_EQ(
      udfApi->getAttribute(udfId, SaiUdfTraits::Attributes::Offset{}),
      kUdfOffset());
}

TEST_F(UdfApiTest, removeUdf) {
  auto udfMatchId = createUdfMatch();
  auto udfGroupId = createUdfGroup();
  auto udfId = createUdf(udfMatchId, udfGroupId);

  EXPECT_EQ(udfId, fs->udfMatchManager.get(udfId).id);
  udfApi->remove(udfId);
}

TEST_F(UdfApiTest, setUdf) {
  auto udfMatchId = createUdfMatch();
  auto udfGroupId = createUdfGroup();
  auto udfId = createUdf(udfMatchId, udfGroupId);

  EXPECT_EQ(udfId, fs->udfMatchManager.get(udfId).id);

  udfApi->setAttribute(udfId, SaiUdfTraits::Attributes::Base{SAI_UDF_BASE_L3});

  EXPECT_EQ(
      udfApi->getAttribute(udfId, SaiUdfTraits::Attributes::Base{}),
      SAI_UDF_BASE_L3);

  EXPECT_THROW(
      udfApi->setAttribute(
          udfId, SaiUdfTraits::Attributes::UdfMatchId{udfMatchId}),
      SaiApiError);
  EXPECT_THROW(
      udfApi->setAttribute(
          udfId, SaiUdfTraits::Attributes::UdfGroupId{udfGroupId}),
      SaiApiError);
  EXPECT_THROW(
      udfApi->setAttribute(
          udfId, SaiUdfTraits::Attributes::Offset{kUdfOffset()}),
      SaiApiError);
}
