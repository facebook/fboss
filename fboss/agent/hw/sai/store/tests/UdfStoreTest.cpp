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
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class UdfStoreTest : public SaiStoreTest {
 public:
  SaiUdfGroupTraits::CreateAttributes createAttributes(
      sai_udf_group_type_t udfGroupType,
      sai_uint16_t udfGroupLength) {
    SaiUdfGroupTraits::Attributes::Type type{udfGroupType};
    SaiUdfGroupTraits::Attributes::Length length{udfGroupLength};
    return {type, length};
  }

  UdfGroupSaiId createUdfGroup(
      sai_udf_group_type_t udfGroupType,
      sai_uint16_t udfGroupLength) {
    return saiApiTable->udfApi().create<SaiUdfGroupTraits>(
        createAttributes(udfGroupType, udfGroupLength), 0);
  }

  UdfMatchSaiId createUdfMatch(
      const std::pair<sai_uint16_t, sai_uint16_t>& l2TypePair,
      const std::pair<sai_uint8_t, sai_uint8_t>& l3TypePair,
      const std::pair<sai_uint16_t, sai_uint16_t>& l4DstPortTypePair) {
    SaiUdfMatchTraits::Attributes::L2Type l2Type{AclEntryFieldU16(l2TypePair)};
    SaiUdfMatchTraits::Attributes::L3Type l3Type{AclEntryFieldU8(l3TypePair)};
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
    SaiUdfMatchTraits::Attributes::L4DstPortType l4DstPortType{
        AclEntryFieldU16(l4DstPortTypePair)};
#endif
    return saiApiTable->udfApi().create<SaiUdfMatchTraits>(
        {l2Type,
         l3Type
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
         ,
         l4DstPortType
#endif
        },
        0);
  }

  UdfSaiId createUdf(UdfMatchSaiId udfMatchId, UdfGroupSaiId udfGroupId) {
    SaiUdfTraits::Attributes::UdfMatchId udfMatchIdAttr{udfMatchId};
    SaiUdfTraits::Attributes::UdfGroupId udfGroupIdAttr{udfGroupId};
    SaiUdfTraits::Attributes::Base baseAttr{SAI_UDF_BASE_L2};
    SaiUdfTraits::Attributes::Offset offsetAttr{kUdfOffset()};
    return saiApiTable->udfApi().create<SaiUdfTraits>(
        {udfMatchIdAttr, udfGroupIdAttr, baseAttr, offsetAttr}, 0);
  }

  SaiUdfMatchTraits::AdapterHostKey udfMatchAdapterHostKey(
      const std::pair<sai_uint16_t, sai_uint16_t>& l2TypePair,
      const std::pair<sai_uint8_t, sai_uint8_t>& l3TypePair,
      const std::pair<sai_uint16_t, sai_uint16_t>& l4DstPortTypePair) {
    SaiUdfMatchTraits::Attributes::L2Type l2Type{AclEntryFieldU16(l2TypePair)};
    SaiUdfMatchTraits::Attributes::L3Type l3Type{AclEntryFieldU8(l3TypePair)};
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
    SaiUdfMatchTraits::Attributes::L4DstPortType l4DstPortType{
        AclEntryFieldU16(l4DstPortTypePair)};
#endif
    return SaiUdfMatchTraits::AdapterHostKey{
        l2Type,
        l3Type
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
        ,
        l4DstPortType
#endif
    };
  }

  SaiUdfTraits::AdapterHostKey udfAdapterHostKey(
      UdfMatchSaiId udfMatchId,
      UdfGroupSaiId udfGroupId) {
    SaiUdfTraits::Attributes::UdfMatchId udfMatchIdAttr{udfMatchId};
    SaiUdfTraits::Attributes::UdfGroupId udfGroupIdAttr{udfGroupId};
    SaiUdfTraits::Attributes::Base baseAttr{SAI_UDF_BASE_L2};
    SaiUdfTraits::Attributes::Offset offsetAttr{kUdfOffset()};
    return SaiUdfTraits::AdapterHostKey{
        udfMatchIdAttr, udfGroupIdAttr, baseAttr, offsetAttr};
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
};

TEST_F(UdfStoreTest, loadUdfGroup) {
  auto udfGroupId = createUdfGroup(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength());
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiUdfGroupTraits>();

  SaiUdfGroupTraits::AdapterHostKey k{"udfGroup"};

  EXPECT_EQ(store.get(k)->adapterKey(), udfGroupId);
}

TEST_F(UdfStoreTest, loadUdfGroup2) {
  SaiUdfGroupTraits::AdapterHostKey k0{"udfGroup0"};
  SaiUdfGroupTraits::AdapterHostKey k1{"udfGroup1"};

  SaiStore s(0);
  auto& store = s.get<SaiUdfGroupTraits>();
  auto obj0 = store.setObject(
      k0, createAttributes(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength()));
  auto obj1 = store.setObject(
      k1, createAttributes(SAI_UDF_GROUP_TYPE_GENERIC, kUdfGroupLength() + 1));

  EXPECT_EQ(store.get(k0)->adapterKey(), obj0->adapterKey());
  EXPECT_EQ(store.get(k1)->adapterKey(), obj1->adapterKey());

  auto adapterKeys = s.adapterKeysFollyDynamic();
  auto adapterKeys2AdapterHostKeys =
      s.adapterKeys2AdapterHostKeysFollyDynamic();
  SaiStore s1(0);
  s1.reload(&adapterKeys, &adapterKeys2AdapterHostKeys);
  auto& store1 = s.get<SaiUdfGroupTraits>();

  EXPECT_EQ(store1.get(k0)->adapterKey(), obj0->adapterKey());
  EXPECT_EQ(store1.get(k1)->adapterKey(), obj1->adapterKey());
}

TEST_F(UdfStoreTest, udfGroupCtor) {
  auto udfGroupId = createUdfGroup(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength());
  SaiUdfGroupTraits::AdapterHostKey k{"udfGroup"};
  auto obj = createObj<SaiUdfGroupTraits>(udfGroupId, k);
  EXPECT_EQ(obj.adapterKey(), udfGroupId);
}

TEST_F(UdfStoreTest, udfGroupCreateCtor) {
  SaiUdfGroupTraits::CreateAttributes c{
      SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength()};
  SaiUdfGroupTraits::AdapterHostKey k{"udfGroup"};
  auto obj = createObj<SaiUdfGroupTraits>(k, c, 0);
  EXPECT_EQ(
      GET_OPT_ATTR(UdfGroup, Type, obj.attributes()), SAI_UDF_GROUP_TYPE_HASH);
  EXPECT_EQ(GET_ATTR(UdfGroup, Length, obj.attributes()), kUdfGroupLength());
}

TEST_F(UdfStoreTest, serDesUdfGroupStore) {
  auto udfGroupId = createUdfGroup(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength());
  verifyAdapterKeySerDeser<SaiUdfGroupTraits>({udfGroupId});
}

TEST_F(UdfStoreTest, toStrUdfGroupStore) {
  createUdfGroup(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength());
  verifyToStr<SaiUdfGroupTraits>();
}

TEST_F(UdfStoreTest, loadUdfMatch) {
  auto udfMatchId0 = createUdfMatch(kL2Type(), kL3Type(), kL4DstPort());
  auto udfMatchId1 = createUdfMatch(kL2Type2(), kL3Type2(), kL4DstPort2());
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiUdfMatchTraits>();

  auto k0 = udfMatchAdapterHostKey(kL2Type(), kL3Type(), kL4DstPort());
  auto k1 = udfMatchAdapterHostKey(kL2Type2(), kL3Type2(), kL4DstPort2());

  EXPECT_EQ(store.get(k0)->adapterKey(), udfMatchId0);
  EXPECT_EQ(store.get(k1)->adapterKey(), udfMatchId1);
}

TEST_F(UdfStoreTest, udfMatchCtor) {
  auto udfMatchId = createUdfMatch(kL2Type(), kL3Type(), kL4DstPort());
  auto obj = createObj<SaiUdfMatchTraits>(udfMatchId);
  EXPECT_EQ(obj.adapterKey(), udfMatchId);
}

TEST_F(UdfStoreTest, udfMatchCreateCtor) {
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  SaiUdfMatchTraits::CreateAttributes c{kL2Type(), kL3Type(), kL4DstPort()};
#else
  SaiUdfMatchTraits::CreateAttributes c{kL2Type(), kL3Type()};
#endif
  auto k = udfMatchAdapterHostKey(kL2Type(), kL3Type(), kL4DstPort());
  auto obj = createObj<SaiUdfMatchTraits>(k, c, 0);
  EXPECT_EQ(
      GET_ATTR(UdfMatch, L2Type, obj.attributes()),
      AclEntryFieldU16(kL2Type()));
  EXPECT_EQ(
      GET_ATTR(UdfMatch, L3Type, obj.attributes()), AclEntryFieldU8(kL3Type()));
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  EXPECT_EQ(
      GET_ATTR(UdfMatch, L4DstPortType, obj.attributes()),
      AclEntryFieldU16(kL4DstPort()));
#endif
}

TEST_F(UdfStoreTest, serDesUdfMatchStore) {
  auto udfMatchId = createUdfMatch(kL2Type(), kL3Type(), kL4DstPort());
  verifyAdapterKeySerDeser<SaiUdfMatchTraits>({udfMatchId});
}

TEST_F(UdfStoreTest, toStrUdfMatchStore) {
  createUdfMatch(kL2Type(), kL3Type(), kL4DstPort());
  verifyToStr<SaiUdfMatchTraits>();
}

TEST_F(UdfStoreTest, loadUdf) {
  auto udfMatchId0 = createUdfMatch(kL2Type(), kL3Type(), kL4DstPort());
  auto udfMatchId1 = createUdfMatch(kL2Type2(), kL3Type2(), kL4DstPort2());
  auto udfGroupId = createUdfGroup(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength());
  auto udfId0 = createUdf(udfMatchId0, udfGroupId);
  auto udfId1 = createUdf(udfMatchId1, udfGroupId);
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiUdfTraits>();

  auto k0 = udfAdapterHostKey(udfMatchId0, udfGroupId);
  auto k1 = udfAdapterHostKey(udfMatchId1, udfGroupId);

  EXPECT_EQ(store.get(k0)->adapterKey(), udfId0);
  EXPECT_EQ(store.get(k1)->adapterKey(), udfId1);
}

TEST_F(UdfStoreTest, udfCtor) {
  auto udfMatchId = createUdfMatch(kL2Type(), kL3Type(), kL4DstPort());
  auto udfGroupId = createUdfGroup(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength());
  auto udfId = createUdf(udfMatchId, udfGroupId);
  auto obj = createObj<SaiUdfTraits>(udfId);
  EXPECT_EQ(obj.adapterKey(), udfId);
}

TEST_F(UdfStoreTest, udfCreateCtor) {
  auto udfMatchId = createUdfMatch(kL2Type(), kL3Type(), kL4DstPort());
  auto udfGroupId = createUdfGroup(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength());
  SaiUdfTraits::CreateAttributes c{
      udfMatchId, udfGroupId, SAI_UDF_BASE_L2, kUdfOffset()};
  auto k = udfAdapterHostKey(udfMatchId, udfGroupId);
  auto obj = createObj<SaiUdfTraits>(k, c, 0);
  EXPECT_EQ(GET_ATTR(Udf, UdfMatchId, obj.attributes()), udfMatchId);
  EXPECT_EQ(GET_ATTR(Udf, UdfGroupId, obj.attributes()), udfGroupId);
  EXPECT_EQ(GET_OPT_ATTR(Udf, Base, obj.attributes()), SAI_UDF_BASE_L2);
  EXPECT_EQ(GET_ATTR(Udf, Offset, obj.attributes()), kUdfOffset());
}

TEST_F(UdfStoreTest, serDesUdfStore) {
  auto udfMatchId = createUdfMatch(kL2Type(), kL3Type(), kL4DstPort());
  auto udfGroupId = createUdfGroup(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength());
  auto udfId = createUdf(udfMatchId, udfGroupId);
  verifyAdapterKeySerDeser<SaiUdfTraits>({udfId});
}

TEST_F(UdfStoreTest, toStrUdfStore) {
  auto udfMatchId = createUdfMatch(kL2Type(), kL3Type(), kL4DstPort());
  auto udfGroupId = createUdfGroup(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength());
  createUdf(udfMatchId, udfGroupId);
  verifyToStr<SaiUdfTraits>();
}
