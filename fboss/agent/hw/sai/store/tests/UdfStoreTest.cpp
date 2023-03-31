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
  UdfGroupSaiId createUdfGroup(
      sai_udf_group_type_t udfGroupType,
      sai_uint16_t udfGroupLength) {
    SaiUdfGroupTraits::Attributes::Type type{udfGroupType};
    SaiUdfGroupTraits::Attributes::Length length{udfGroupLength};
    return saiApiTable->udfApi().create<SaiUdfGroupTraits>({type, length}, 0);
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
        {
          l2Type, l3Type
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

  SaiUdfGroupTraits::AdapterHostKey udfGroupAdapterHostKey(
      sai_udf_group_type_t udfGroupType,
      sai_uint16_t udfGroupLength) {
    SaiUdfGroupTraits::Attributes::Type type{udfGroupType};
    SaiUdfGroupTraits::Attributes::Length length{udfGroupLength};
    return SaiUdfGroupTraits::AdapterHostKey{type, length};
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
    return SaiUdfMatchTraits::AdapterHostKey {
      l2Type, l3Type
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
  auto udfGroupId0 = createUdfGroup(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength());
  auto udfGroupId1 =
      createUdfGroup(SAI_UDF_GROUP_TYPE_GENERIC, kUdfGroupLength() + 1);
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiUdfGroupTraits>();

  auto k0 = udfGroupAdapterHostKey(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength());
  auto k1 =
      udfGroupAdapterHostKey(SAI_UDF_GROUP_TYPE_GENERIC, kUdfGroupLength() + 1);

  EXPECT_EQ(store.get(k0)->adapterKey(), udfGroupId0);
  EXPECT_EQ(store.get(k1)->adapterKey(), udfGroupId1);
}

TEST_F(UdfStoreTest, udfGroupCtor) {
  auto udfGroupId = createUdfGroup(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength());
  auto obj = createObj<SaiUdfGroupTraits>(udfGroupId);
  EXPECT_EQ(obj.adapterKey(), udfGroupId);
}

TEST_F(UdfStoreTest, udfGroupCreateCtor) {
  SaiUdfGroupTraits::CreateAttributes c{
      SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength()};
  auto k = udfGroupAdapterHostKey(SAI_UDF_GROUP_TYPE_HASH, kUdfGroupLength());
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
