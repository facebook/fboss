/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

template <typename T>
using A = SaiAttribute<sai_attr_id_t, 0, T>;
using ObjAttr = SaiAttribute<sai_attr_id_t, 0, sai_object_id_t, SaiObjectIdT>;
using UInt64TAttr = SaiAttribute<sai_attr_id_t, 0, sai_uint64_t>;
using VecAttr = SaiAttribute<
    sai_attr_id_t,
    0,
    sai_object_list_t,
    std::vector<sai_object_id_t>>;
using MacAttr = SaiAttribute<sai_attr_id_t, 0, sai_mac_t, folly::MacAddress>;
using B = A<bool>;
using I = A<int32_t>;
using AttrTuple = SaiAttributeTuple<B, I, VecAttr>;
using AttrOptional = SaiAttributeOptional<I>;
static_assert(
    std::is_same<
        typename AttrTuple::ValueType,
        std::tuple<bool, int32_t, std::vector<sai_object_id_t>>>::value,
    "SaiAttributeTuple::ValueType is not tuple of SaiAttribute ValueTypes");
using AttrVariant = SaiAttributeVariant<B, I, VecAttr>;
static_assert(
    std::is_same<
        typename AttrVariant::ValueType,
        boost::variant<bool, int32_t, std::vector<sai_object_id_t>>>::value,
    "SaiAttributeVariant::ValueType is not tuple of SaiAttribute ValueTypes");


template <typename AttrT>
typename AttrT::ValueType getConstAttrValue(const AttrT& a) {
  return a.value();
}

TEST(Attribute, getBool) {
  A<bool> a;
  a.saiAttr()->value.booldata = true;
  EXPECT_TRUE(a.value());
  a.saiAttr()->value.booldata = false;
  EXPECT_FALSE(a.value());
}

TEST(Attribute, ctorBool) {
  A<bool> a(true);
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(a.saiAttr()->value.booldata);
}

TEST(Attribute, modifyBoolUnderneath) {
  A<bool> a(true);
  EXPECT_TRUE(a.value());
  a.saiAttr()->value.booldata = false;
  EXPECT_FALSE(a.value());
}

TEST(Attribute, constBoolAttribute) {
  A<bool> a(true);
  EXPECT_TRUE(getConstAttrValue(a));
  a.saiAttr()->value.booldata = false;
  EXPECT_FALSE(getConstAttrValue(a));
}

TEST(Attribute, fnReturnBool) {
  auto fn = []() {
    A<bool> a(true);
    return a;
  };
  EXPECT_TRUE(fn().value());
}

void fill_in_deadbeef(sai_mac_t& mac) {
  mac[0] = 0xDE;
  mac[1] = 0xAD;
  mac[2] = 0xBE;
  mac[3] = 0xEF;
  mac[4] = 0x42;
  mac[5] = 0x42;
}

void expect_deadbeef(const sai_mac_t& mac) {
  EXPECT_EQ(mac[0], 0xDE);
  EXPECT_EQ(mac[1], 0xAD);
  EXPECT_EQ(mac[2], 0xBE);
  EXPECT_EQ(mac[3], 0xEF);
  EXPECT_EQ(mac[4], 0x42);
  EXPECT_EQ(mac[5], 0x42);
}

TEST(Attribute, getMac) {
  folly::MacAddress mac("DE:AD:BE:EF:42:42");
  MacAttr a;
  fill_in_deadbeef(a.saiAttr()->value.mac);
  EXPECT_EQ(a.value(), mac);
}

TEST(Attribute, ctorMac) {
  folly::MacAddress mac("DE:AD:BE:EF:42:42");
  MacAttr a(mac);
  EXPECT_EQ(a.value(), mac);
  expect_deadbeef(a.saiAttr()->value.mac);
}

// MacAttr/mac going out of scope should not affect macs
// extracted from it with value()
TEST(Attribute, lifetimeMac) {
  folly::MacAddress mac2;
  {
    folly::MacAddress mac("DE:AD:BE:EF:42:42");
    MacAttr a(mac);
    EXPECT_EQ(a.value(), mac);
    expect_deadbeef(a.saiAttr()->value.mac);
    mac2 = a.value();
  }
  folly::MacAddress mac("DE:AD:BE:EF:42:42");
  EXPECT_EQ(mac2, mac);
}

// MacAttr going out of scope should not affect MacAttrs
// copied from it, nor macs copied into it
TEST(Attribute, lifetimeMacAttr) {
  folly::MacAddress mac("DE:AD:BE:EF:42:42");
  MacAttr a2;
  {
    MacAttr a(mac);
    EXPECT_EQ(a.value(), mac);
    expect_deadbeef(a.saiAttr()->value.mac);
    a2 = a;
  }
  folly::MacAddress mac2("DE:AD:BE:EF:42:42");
  EXPECT_EQ(mac2, mac);
  EXPECT_EQ(mac2, a2.value());
}

TEST(Attribute, fnReturnMac) {
  auto fn = []() {
    folly::MacAddress mac("DE:AD:BE:EF:42:42");
    MacAttr a(mac);
    return a;
  };
  folly::MacAddress mac("DE:AD:BE:EF:42:42");
  auto a = fn();
  EXPECT_EQ(a.value(), mac);
  expect_deadbeef(a.saiAttr()->value.mac);
}

TEST(Attribute, constMacAttribute) {
  folly::MacAddress mac("DE:AD:BE:EF:42:42");
  MacAttr a;
  fill_in_deadbeef(a.saiAttr()->value.mac);
  EXPECT_EQ(getConstAttrValue(a), mac);
}

TEST(Attribute, getVec) {
  std::vector<sai_object_id_t> v;
  v.resize(4);
  VecAttr a(v);
  a.saiAttr()->value.objlist.count = 4;
  for (int i = 0; i < 4; ++i) {
    a.saiAttr()->value.objlist.list[i] = i * i;
  }
  std::vector<sai_object_id_t> v2 = a.value();
  EXPECT_EQ(v2.size(), 4);
  for (int j = 0; j < 4; ++j) {
    EXPECT_EQ(v2.at(j), j * j);
  }
}

TEST(Attribute, ctorVec) {
  std::vector<sai_object_id_t> v{0, 1, 4, 9};
  VecAttr a(v);
  for (int i = 0; i < a.saiAttr()->value.objlist.count; ++i) {
    EXPECT_EQ(a.saiAttr()->value.objlist.list[i], i * i);
  }
}

TEST(Attribute, lifetimeVec) {
  std::vector<sai_object_id_t> v2;
  {
    std::vector<sai_object_id_t> v{0, 1, 4, 9};
    VecAttr a(v);
    v2 = a.value();
  }
  std::vector<sai_object_id_t> v{0, 1, 4, 9};
  EXPECT_EQ(v2, v);
}

TEST(Attribute, lifetimeVecAttr) {
  std::vector<sai_object_id_t> v{0, 1, 4, 9};
  VecAttr a2(v);
  VecAttr a3;
  {
    VecAttr a(v);
    a3 = a;
  }
  std::vector<sai_object_id_t> v2{0, 1, 4, 9};
  EXPECT_EQ(v2, v);
  EXPECT_EQ(v2, a2.value());
  EXPECT_EQ(v2, a3.value());
}

TEST(Attribute, dupeType) {
  ObjAttr oa(42);
  EXPECT_EQ(oa.value(), 42);
  static_assert(
      std::is_same<ObjAttr::ValueType, uint64_t>::value,
      "sai_object_id_t attr ValueT should be uint64_t");
  UInt64TAttr ua(43);
  EXPECT_EQ(ua.value(), 43);
  static_assert(
      std::is_same<UInt64TAttr::ValueType, uint64_t>::value,
      "uint64_t attr ValueT should be uint64_t");
}

TEST(Attribute, attributeTupleGetSaiAttr) {
  std::vector<sai_object_id_t> vec;
  vec.resize(42);
  AttrTuple at{B{true}, I{42}, VecAttr{vec}};
  std::vector<sai_attribute_t> saiAttrs = at.saiAttrs();
  EXPECT_EQ(saiAttrs.size(), 3);
  EXPECT_TRUE(saiAttrs[0].value.booldata);
  EXPECT_EQ(saiAttrs[1].value.s32, 42);
  EXPECT_EQ(saiAttrs[2].value.objlist.count, 42);
}

TEST(Attribute, attributeTupleGetValue) {
  std::vector<sai_object_id_t> vec;
  vec.resize(42);
  AttrTuple at{B{true}, I{42}, VecAttr{vec}};
  EXPECT_EQ(std::get<0>(at.value()), true);
  EXPECT_EQ(std::get<1>(at.value()), 42);
  EXPECT_EQ(std::get<2>(at.value()).size(), 42);
  EXPECT_EQ(at.value(), std::make_tuple(true, 42, vec));
}

TEST(Attribute, attributeVariantGetSaiAttr) {
  AttrVariant v1{B{true}};
  AttrVariant v2{I{42}};
  std::vector<sai_object_id_t> vec;
  vec.resize(42);
  AttrVariant v3{VecAttr{vec}};
  std::vector<sai_attribute_t> saiAttrs1 = v1.saiAttrs();
  EXPECT_EQ(saiAttrs1.size(), 1);
  std::vector<sai_attribute_t> saiAttrs2 = v2.saiAttrs();
  EXPECT_EQ(saiAttrs2.size(), 1);
  std::vector<sai_attribute_t> saiAttrs3 = v3.saiAttrs();
  EXPECT_EQ(saiAttrs3.size(), 1);
  EXPECT_TRUE(saiAttrs1[0].value.booldata);
  EXPECT_EQ(saiAttrs2[0].value.s32, 42);
  EXPECT_EQ(saiAttrs3[0].value.objlist.count, 42);
}

TEST(Attribute, attributeAVariantGetValue) {
  AttrVariant v1{B{true}};
  AttrVariant v2{I{42}};
  std::vector<sai_object_id_t> vec;
  vec.resize(42);
  AttrVariant v3{VecAttr{vec}};
  EXPECT_TRUE(boost::get<B::ValueType>(v1.value()));
  EXPECT_EQ(boost::get<I::ValueType>(v2.value()), 42);
  EXPECT_EQ(boost::get<VecAttr::ValueType>(v3.value()).size(), 42);
}

TEST(Attribute, attributeAVariantGetValueParameterized) {
  AttrVariant v1{B{true}};
  AttrVariant v2{I{42}};
  std::vector<sai_object_id_t> vec;
  vec.resize(42);
  AttrVariant v3{VecAttr{vec}};
  EXPECT_TRUE(v1.value<B>());
  EXPECT_EQ(v2.value<I>(), 42);
  EXPECT_EQ(v3.value<VecAttr>().size(), 42);
}

TEST(Attribute, attributeAVariantGetValueWrongType) {
  AttrVariant v1{B{true}};
  AttrVariant v2{I{42}};
  std::vector<sai_object_id_t> vec;
  vec.resize(42);
  AttrVariant v3{VecAttr{vec}};
  EXPECT_THROW(v1.value<I>(), boost::bad_get);
  EXPECT_THROW(v1.value<VecAttr>(), boost::bad_get);
  EXPECT_THROW(v2.value<B>(), boost::bad_get);
  EXPECT_THROW(v2.value<VecAttr>(), boost::bad_get);
  EXPECT_THROW(v3.value<B>(), boost::bad_get);
  EXPECT_THROW(v3.value<I>(), boost::bad_get);
  /*
   * The implementations of these use boost::get<TYPE>() which has a compile
   * time check for access to a type that isn't present in the variant entirely
  EXPECT_THROW(v1.value<ObjAttr>(), boost::bad_get);
  */
}

TEST(Attribute, attributeVariantDupMembers) {
  using A1 = SaiAttribute<sai_attr_id_t, 0, uint64_t>;
  using A2 = SaiAttribute<sai_attr_id_t, 1, uint64_t>;
  using AV = SaiAttributeVariant<A1, A2>;
  AV av1{A1{42}};
  AV av2{A2{420}};
  EXPECT_EQ(av1.value<A1>(), 42);
  EXPECT_EQ(av2.value<A2>(), 420);
}

TEST(Attribute, attributeVariantWrongAttributeRightValueType) {
  using A1 = SaiAttribute<sai_attr_id_t, 0, uint64_t>;
  using A2 = SaiAttribute<sai_attr_id_t, 1, uint64_t>;
  using AV = SaiAttributeVariant<A1>;
  AV av1{A1{42}};
  EXPECT_EQ(av1.value<A1>(), 42);
  /*
   * As above, this should fail to compile.
  EXPECT_EQ(av1.value<A2>(), 42);
  */
}

TEST(Attribute, attributeOptionalGetSaiAttr) {
  AttrOptional o(I{42});
  std::vector<sai_attribute_t> v = o.saiAttrs();
  EXPECT_EQ(v.size(), 1);
}

TEST(Attribute, attributeOptionalGetValue) {
  AttrOptional o(I{42});
  folly::Optional<int32_t> v = o.value();
  EXPECT_EQ(v.value(), 42);
}

TEST(Attribute, attributeOptionalGetSaiAttrNone) {
  AttrOptional o;
  EXPECT_EQ(o.value(), folly::none);
}

TEST(Attribute, attributeOptionalGetValueNone) {
  AttrOptional o;
  std::vector<sai_attribute_t> v = o.saiAttrs();
  EXPECT_EQ(v.size(), 0);
}

TEST(Attribute, optionalConstructFromFollyOptional) {
  folly::Optional<int32_t> follyOptional{42};
  AttrOptional o(follyOptional);
  folly::Optional<int32_t> v = o.value();
  EXPECT_EQ(v.value(), 42);
}

TEST(Attribute, optionalConstructFromFollyNone) {
  folly::Optional<int32_t> follyOptional = folly::none;
  AttrOptional o(follyOptional);
  EXPECT_EQ(o.value(), folly::none);
}


// Tests for a mix of various SaiAttribute wrapper types
TEST(Attribute, variantInTuple) {
  AttrVariant v1{I{42}};
  AttrVariant v2{I{420}};
  B b{true};
  SaiAttributeTuple<AttrVariant, AttrVariant, B> t{v1, v2, b};
  typename SaiAttributeTuple<AttrVariant, AttrVariant, B>::ValueType v =
      t.value();
  EXPECT_EQ(boost::get<typename I::ValueType>(std::get<0>(v)), 42);
  EXPECT_EQ(boost::get<typename I::ValueType>(std::get<1>(v)), 420);
  EXPECT_TRUE(std::get<2>(v));
}
TEST(Attribute, vectorInTuple) {
  SaiAttributeTuple<B, VecAttr, I> t(B{true}, VecAttr{{42}}, I{420});
  std::tuple<bool, std::vector<uint64_t>, int32_t> vt = t.value();
  EXPECT_TRUE(std::get<0>(vt));
  EXPECT_EQ(std::get<1>(vt)[0], 42);
  EXPECT_EQ(std::get<2>(vt), 420);
}
TEST(Attribute, optionalInTuple) {
  SaiAttributeTuple<B, AttrOptional, I> t(B{true}, AttrOptional{I{42}}, I{420});
  std::tuple<bool, folly::Optional<int32_t>, int32_t> vt = t.value();
  EXPECT_TRUE(std::get<0>(vt));
  EXPECT_EQ(std::get<1>(vt).value(), 42);
  EXPECT_EQ(std::get<2>(vt), 420);
}
TEST(Attribute, tupleInTuple) {
  SaiAttributeTuple<B, SaiAttributeTuple<B, I>, I> t(
      B{true}, SaiAttributeTuple<B, I>{B{false}, I{42}}, I{420});
  std::tuple<bool, std::tuple<bool, int>, int> vt = t.value();
  EXPECT_TRUE(std::get<0>(vt));
  std::tuple<bool, int> nested = std::get<1>(vt);
  EXPECT_FALSE(std::get<0>(nested));
  EXPECT_EQ(std::get<1>(nested), 42);
  EXPECT_EQ(std::get<2>(vt), 420);
}
TEST(Attribute, tupleInVariant) {
  SaiAttributeVariant<SaiAttributeTuple<B, I>, I> var(
      SaiAttributeTuple<B, I>{B{true}, I{42}});
  boost::variant<std::tuple<bool, int>, int> varv = var.value();
  std::tuple<bool, int> t = boost::get<std::tuple<bool, int>>(varv);
  EXPECT_TRUE(std::get<0>(t));
  EXPECT_EQ(std::get<1>(t), 42);
}
TEST(Attribute, optionalInVariant) {
  SaiAttributeVariant<SaiAttributeOptional<I>, B> var(
      SaiAttributeOptional<I>{I{42}});
  boost::variant<folly::Optional<int>, bool> varv = var.value();
  folly::Optional<int> o = boost::get<folly::Optional<int>>(varv);
  EXPECT_EQ(o.value(), 42);
}
TEST(Attribute, variantInVariant) {
  SaiAttributeVariant<SaiAttributeVariant<B, I>, I> var(
      SaiAttributeVariant<B, I>{I{42}});
  boost::variant<boost::variant<bool, int>, int> varv = var.value();
  boost::variant<bool, int> nested =
      boost::get<boost::variant<bool, int>>(varv);
  EXPECT_EQ(boost::get<int>(nested), 42);
}
TEST(Attribute, tupleInOptional) {
  SaiAttributeOptional<SaiAttributeTuple<B, I>> o(
      SaiAttributeTuple<B, I>{B{true}, I{42}});
  folly::Optional<std::tuple<bool, int>> ov = o.value();
  EXPECT_EQ(std::get<0>(ov.value()), true);
  EXPECT_EQ(std::get<1>(ov.value()), 42);
}
TEST(Attribute, variantInOptional) {
  SaiAttributeOptional<SaiAttributeVariant<B, I>> o(
      SaiAttributeVariant<B, I>{I{42}});
  folly::Optional<boost::variant<bool, int>> ov = o.value();
  EXPECT_EQ(boost::get<int>(ov.value()), 42);
}
TEST(Attribute, optionalInOptional) {
  SaiAttributeOptional<SaiAttributeOptional<I>> o(
      SaiAttributeOptional<I>{I{42}});
  folly::Optional<folly::Optional<int>> ov = o.value();
  EXPECT_EQ(ov.value().value(), 42);
}
