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
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"

#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

namespace {
const auto kFullHash = SaiHashTraits::Attributes::NativeHashFieldList{{
    SAI_NATIVE_HASH_FIELD_SRC_IP,
    SAI_NATIVE_HASH_FIELD_DST_IP,
    SAI_NATIVE_HASH_FIELD_L4_SRC_PORT,
    SAI_NATIVE_HASH_FIELD_L4_DST_PORT,
}};
const auto kHalfHash = SaiHashTraits::Attributes::NativeHashFieldList{{
    SAI_NATIVE_HASH_FIELD_SRC_IP,
    SAI_NATIVE_HASH_FIELD_DST_IP,
}};
} // namespace

class HashStoreTest : public SaiStoreTest {
 public:
  HashSaiId createHash(
      SaiHashTraits::Attributes::NativeHashFieldList nativeFields,
      SaiHashTraits::Attributes::UDFGroupList udf) {
    return saiApiTable->hashApi().create<SaiHashTraits>(
        SaiHashTraits::CreateAttributes{nativeFields, udf}, 0);
  }
};

TEST_F(HashStoreTest, loadFullHash) {
  auto id = createHash(kFullHash, {{}});

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiHashTraits>();

  SaiHashTraits::AdapterHostKey k{
      kFullHash,
      std::optional<SaiHashTraits::Attributes::UDFGroupList>{
          SaiHashTraits::Attributes::UDFGroupList{}}};
  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), id);
}

TEST_F(HashStoreTest, loadHalfHash) {
  auto id = createHash(kHalfHash, {{}});

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiHashTraits>();

  SaiHashTraits::AdapterHostKey k{
      kHalfHash,
      std::optional<SaiHashTraits::Attributes::UDFGroupList>{
          SaiHashTraits::Attributes::UDFGroupList{}}};
  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), id);
}

TEST_F(HashStoreTest, loadNativeAndUdfHash) {
  auto id = createHash(kHalfHash, {{42}});

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiHashTraits>();

  SaiHashTraits::AdapterHostKey k{
      kHalfHash, SaiHashTraits::Attributes::UDFGroupList{{42}}};
  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), id);
}

TEST_F(HashStoreTest, hashLoadCtor) {
  auto hashId = createHash(kFullHash, {{}});
  SaiObject<SaiHashTraits> hashObj(hashId);
  EXPECT_EQ(hashObj.adapterKey(), hashId);
  EXPECT_EQ(
      GET_OPT_ATTR(Hash, NativeHashFieldList, hashObj.attributes()),
      kFullHash.value());
  EXPECT_TRUE(GET_OPT_ATTR(Hash, UDFGroupList, hashObj.attributes()).empty());
}

TEST_F(HashStoreTest, hashCreateCtor) {
  SaiHashTraits::CreateAttributes attrs{
      kFullHash,
      std::optional<SaiHashTraits::Attributes::UDFGroupList>{
          SaiHashTraits::Attributes::UDFGroupList{}}};
  SaiHashTraits::AdapterHostKey adapterHostKey = attrs;
  SaiObject<SaiHashTraits> obj(adapterHostKey, attrs, 0);
  auto nativeHashFields = saiApiTable->hashApi().getAttribute(
      obj.adapterKey(), SaiHashTraits::Attributes::NativeHashFieldList{});
  EXPECT_EQ(nativeHashFields, kFullHash.value());
}

TEST_F(HashStoreTest, hashSetNatveFields) {
  auto id = createHash(kFullHash, {{}});
  SaiObject<SaiHashTraits> hashObj(id);
  EXPECT_EQ(
      GET_OPT_ATTR(Hash, NativeHashFieldList, hashObj.attributes()),
      kFullHash.value());
  hashObj.setAttributes({kHalfHash,
                         std::optional<SaiHashTraits::Attributes::UDFGroupList>{
                             SaiHashTraits::Attributes::UDFGroupList{}}});
  EXPECT_EQ(
      GET_OPT_ATTR(Hash, NativeHashFieldList, hashObj.attributes()),
      kHalfHash.value());
}

TEST_F(HashStoreTest, hashSetUDF) {
  auto id = createHash(kFullHash, {{}});
  SaiObject<SaiHashTraits> hashObj(id);
  EXPECT_EQ(
      GET_OPT_ATTR(Hash, NativeHashFieldList, hashObj.attributes()),
      kFullHash.value());
  SaiHashTraits::Attributes::UDFGroupList udf{{42}};
  hashObj.setAttributes({kFullHash, udf});
  EXPECT_EQ(
      GET_OPT_ATTR(Hash, UDFGroupList, hashObj.attributes()), udf.value());
}

TEST_F(HashStoreTest, serDeser) {
  auto id = createHash(kFullHash, {{}});
  verifyAdapterKeySerDeser<SaiHashTraits>({id});
}

TEST_F(HashStoreTest, toStr) {
  std::ignore = createHash(kFullHash, {{}});
  verifyToStr<SaiHashTraits>();
}
