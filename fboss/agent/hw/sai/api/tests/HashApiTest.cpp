/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/HashApi.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class HashApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    hashApi = std::make_unique<HashApi>();
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<HashApi> hashApi;
};

TEST_F(HashApiTest, emptyHash) {
  auto hashid = hashApi->create<SaiHashTraits>(
      SaiHashTraits::CreateAttributes{
          std::optional<SaiHashTraits::Attributes::NativeHashFieldList>{},
          std::nullopt},
      0);
  auto dummy1 = SaiHashTraits::Attributes::NativeHashFieldList{};
  auto hashFields = hashApi->getAttribute(
      hashid, SaiHashTraits::Attributes::NativeHashFieldList{});
  auto udfFields =
      hashApi->getAttribute(hashid, SaiHashTraits::Attributes::UDFGroupList{});
  EXPECT_EQ(hashFields.size(), 0);
  EXPECT_EQ(udfFields.size(), 0);
}

TEST_F(HashApiTest, fullHash) {
  auto hashFields = SaiHashTraits::Attributes::NativeHashFieldList{{
      SAI_NATIVE_HASH_FIELD_SRC_IP,
      SAI_NATIVE_HASH_FIELD_DST_IP,
      SAI_NATIVE_HASH_FIELD_L4_SRC_PORT,
      SAI_NATIVE_HASH_FIELD_L4_DST_PORT,
  }};
  auto hashid = hashApi->create<SaiHashTraits>(
      SaiHashTraits::CreateAttributes{hashFields, std::nullopt}, 0);
  auto nativeHashFieldsGot = hashApi->getAttribute(
      hashid, SaiHashTraits::Attributes::NativeHashFieldList{});
  auto userDefinedFieldsGot =
      hashApi->getAttribute(hashid, SaiHashTraits::Attributes::UDFGroupList{});
  EXPECT_EQ(nativeHashFieldsGot.size(), 4);
  EXPECT_EQ(userDefinedFieldsGot.size(), 0);
}

TEST_F(HashApiTest, halfHash) {
  auto hashFields = SaiHashTraits::Attributes::NativeHashFieldList{{
      SAI_NATIVE_HASH_FIELD_SRC_IP,
      SAI_NATIVE_HASH_FIELD_DST_IP,
  }};
  auto hashid = hashApi->create<SaiHashTraits>(
      SaiHashTraits::CreateAttributes{hashFields, std::nullopt}, 0);
  auto nativeHashFieldsGot = hashApi->getAttribute(
      hashid, SaiHashTraits::Attributes::NativeHashFieldList{});
  auto userDefinedFieldsGot =
      hashApi->getAttribute(hashid, SaiHashTraits::Attributes::UDFGroupList{});
  EXPECT_EQ(nativeHashFieldsGot.size(), 2);
  EXPECT_EQ(userDefinedFieldsGot.size(), 0);
}

TEST_F(HashApiTest, hashAndUdf) {
  auto hashFields = SaiHashTraits::Attributes::NativeHashFieldList{{
      SAI_NATIVE_HASH_FIELD_SRC_IP,
      SAI_NATIVE_HASH_FIELD_DST_IP,
  }};
  auto udfGroups = SaiHashTraits::Attributes::UDFGroupList{{42}};
  auto hashid = hashApi->create<SaiHashTraits>(
      SaiHashTraits::CreateAttributes{hashFields, udfGroups}, 0);
  auto nativeHashFieldsGot = hashApi->getAttribute(
      hashid, SaiHashTraits::Attributes::NativeHashFieldList{});
  auto userDefinedFieldsGot =
      hashApi->getAttribute(hashid, SaiHashTraits::Attributes::UDFGroupList{});
  EXPECT_EQ(nativeHashFieldsGot.size(), 2);
  EXPECT_EQ(userDefinedFieldsGot.size(), 1);
}

TEST_F(HashApiTest, setHash) {
  auto hashid = hashApi->create<SaiHashTraits>(
      SaiHashTraits::CreateAttributes{std::nullopt, std::nullopt}, 0);
  auto hashFields = SaiHashTraits::Attributes::NativeHashFieldList{
      {SAI_NATIVE_HASH_FIELD_SRC_IP}};
  hashApi->setAttribute(hashid, hashFields);
  auto nativeHashFieldsGot = hashApi->getAttribute(
      hashid, SaiHashTraits::Attributes::NativeHashFieldList{});
  auto userDefinedFieldsGot =
      hashApi->getAttribute(hashid, SaiHashTraits::Attributes::UDFGroupList{});
  EXPECT_EQ(nativeHashFieldsGot.size(), 1);
  EXPECT_EQ(nativeHashFieldsGot[0], SAI_NATIVE_HASH_FIELD_SRC_IP);
  EXPECT_EQ(userDefinedFieldsGot.size(), 0);
}

TEST_F(HashApiTest, setUdf) {
  auto hashid = hashApi->create<SaiHashTraits>(
      SaiHashTraits::CreateAttributes{std::nullopt, std::nullopt}, 0);
  auto udfFields = SaiHashTraits::Attributes::UDFGroupList{{42}};
  hashApi->setAttribute(hashid, udfFields);
  auto nativeHashFieldsGot = hashApi->getAttribute(
      hashid, SaiHashTraits::Attributes::NativeHashFieldList{});
  auto userDefinedFieldsGot =
      hashApi->getAttribute(hashid, SaiHashTraits::Attributes::UDFGroupList{});
  EXPECT_EQ(nativeHashFieldsGot.size(), 0);
  EXPECT_EQ(userDefinedFieldsGot.size(), 1);
  EXPECT_EQ(userDefinedFieldsGot[0], 42);
}

TEST_F(HashApiTest, nativeFieldHashTest) {
  auto halfHash = SaiHashTraits::Attributes::NativeHashFieldList{
      {SAI_NATIVE_HASH_FIELD_SRC_IP, SAI_NATIVE_HASH_FIELD_DST_IP}};

  auto fullHash = SaiHashTraits::Attributes::NativeHashFieldList{
      {SAI_NATIVE_HASH_FIELD_SRC_IP,
       SAI_NATIVE_HASH_FIELD_DST_IP,
       SAI_NATIVE_HASH_FIELD_L4_SRC_PORT,
       SAI_NATIVE_HASH_FIELD_L4_DST_PORT}};

  auto halfHashKey = SaiHashTraits::AdapterHostKey{halfHash, std::nullopt};
  auto fullHashKey = SaiHashTraits::AdapterHostKey{fullHash, std::nullopt};

  EXPECT_NE(
      std::hash<SaiHashTraits::AdapterHostKey>{}(halfHashKey),
      std::hash<SaiHashTraits::AdapterHostKey>{}(fullHashKey));
  EXPECT_EQ(
      std::hash<SaiHashTraits::AdapterHostKey>{}(halfHashKey),
      std::hash<SaiHashTraits::AdapterHostKey>{}(halfHashKey));
  EXPECT_EQ(
      std::hash<SaiHashTraits::AdapterHostKey>{}(fullHashKey),
      std::hash<SaiHashTraits::AdapterHostKey>{}(fullHashKey));
}

TEST_F(HashApiTest, emptyFieldHashTest) {
  auto emptyHash = SaiHashTraits::AdapterHostKey{std::nullopt, std::nullopt};

  EXPECT_EQ(std::hash<SaiHashTraits::AdapterHostKey>{}(emptyHash), 0);
}

TEST_F(HashApiTest, formatHashNativeHashFieldList) {
  SaiHashTraits::Attributes::NativeHashFieldList nhfl{{
      SAI_NATIVE_HASH_FIELD_SRC_IP,
      SAI_NATIVE_HASH_FIELD_DST_IP,
      SAI_NATIVE_HASH_FIELD_L4_SRC_PORT,
      SAI_NATIVE_HASH_FIELD_L4_DST_PORT,
  }};
  std::string expected("NativeHashFieldList: {0, 1, 7, 8}");
  EXPECT_EQ(expected, fmt::format("{}", nhfl));
}

TEST_F(HashApiTest, formatUDFGroupList) {
  SaiHashTraits::Attributes::UDFGroupList udfgl{{42}};
  std::string expected("UDFGroupList: {42}");
  EXPECT_EQ(expected, fmt::format("{}", udfgl));
}
