/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"

using namespace facebook::fboss;

template <typename T>
using A = SaiAttribute<sai_attr_id_t, 0, T>;
using VecAttr = SaiAttribute<sai_attr_id_t, 0, std::vector<sai_object_id_t>>;
using B = A<bool>;
using I = A<int32_t>;
using AttrTuple = SaiAttributeTuple<B, I, VecAttr>;
using AttrOptional = SaiAttributeOptional<I>;
using AttrVariant = SaiAttributeVariant<B, I, VecAttr>;
TEST(AttributeDataTypes, attributeTupleGetSaiAttr) {
  std::vector<sai_object_id_t> vec;
  vec.resize(42);
  AttrTuple at{B{true}, I{42}, VecAttr{vec}};
  std::vector<sai_attribute_t> saiAttrs = at.saiAttrs();
  EXPECT_EQ(saiAttrs.size(), 3);
  EXPECT_TRUE(saiAttrs[0].value.booldata);
  EXPECT_EQ(saiAttrs[1].value.s32, 42);
  EXPECT_EQ(saiAttrs[2].value.objlist.count, 42);
}

TEST(AttributeDataTypes, attributeTupleGetValue) {
  std::vector<sai_object_id_t> vec;
  vec.resize(42);
  AttrTuple at{B{true}, I{42}, VecAttr{vec}};
  EXPECT_EQ(std::get<0>(at.value()), true);
  EXPECT_EQ(std::get<1>(at.value()), 42);
  EXPECT_EQ(std::get<2>(at.value()).size(), 42);
  EXPECT_EQ(at.value(), std::make_tuple(true, 42, vec));
}

TEST(AttributeDataTypes, attributeVariantGetSaiAttr) {
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

TEST(AttributeDataTypes, attributeAVariantGetValue) {
  AttrVariant v1{B{true}};
  AttrVariant v2{I{42}};
  std::vector<sai_object_id_t> vec;
  vec.resize(42);
  AttrVariant v3{VecAttr{vec}};
  EXPECT_TRUE(boost::get<B::ValueType>(v1.value()));
  EXPECT_EQ(boost::get<I::ValueType>(v2.value()), 42);
  EXPECT_EQ(boost::get<VecAttr::ValueType>(v3.value()).size(), 42);
}

TEST(AttributeDataTypes, attributeAVariantGetValueParameterized) {
  AttrVariant v1{B{true}};
  AttrVariant v2{I{42}};
  std::vector<sai_object_id_t> vec;
  vec.resize(42);
  AttrVariant v3{VecAttr{vec}};
  EXPECT_TRUE(v1.value<B>());
  EXPECT_EQ(v2.value<I>(), 42);
  EXPECT_EQ(v3.value<VecAttr>().size(), 42);
}

TEST(AttributeDataTypes, attributeAVariantGetValueWrongType) {
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

TEST(AttributeDataTypes, attributeVariantDupMembers) {
  using A1 = SaiAttribute<sai_attr_id_t, 0, uint64_t>;
  using A2 = SaiAttribute<sai_attr_id_t, 1, uint64_t>;
  using AV = SaiAttributeVariant<A1, A2>;
  AV av1{A1{42}};
  AV av2{A2{420}};
  EXPECT_EQ(av1.value<A1>(), 42);
  EXPECT_EQ(av2.value<A2>(), 420);
}

TEST(AttributeDataTypes, attributeVariantWrongAttributeRightValueType) {
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

TEST(AttributeDataTypes, attributeOptionalGetSaiAttr) {
  AttrOptional o(I{42});
  std::vector<sai_attribute_t> v = o.saiAttrs();
  EXPECT_EQ(v.size(), 1);
}

TEST(AttributeDataTypes, attributeOptionalGetValue) {
  AttrOptional o(I{42});
  std::optional<int32_t> v = o.value();
  EXPECT_EQ(v.value(), 42);
}

TEST(AttributeDataTypes, attributeOptionalGetSaiAttrNone) {
  AttrOptional o;
  EXPECT_EQ(o.value(), std::nullopt);
}

TEST(AttributeDataTypes, attributeOptionalGetValueNone) {
  AttrOptional o;
  std::vector<sai_attribute_t> v = o.saiAttrs();
  EXPECT_EQ(v.size(), 0);
}

TEST(AttributeDataTypes, optionalConstructFromFollyOptional) {
  std::optional<int32_t> follyOptional{42};
  AttrOptional o(follyOptional);
  std::optional<int32_t> v = o.value();
  EXPECT_EQ(v.value(), 42);
}

TEST(AttributeDataTypes, optionalConstructFromFollyNone) {
  std::optional<int32_t> follyOptional = std::nullopt;
  AttrOptional o(follyOptional);
  EXPECT_EQ(o.value(), std::nullopt);
}

// Tests for a mix of various SaiAttribute wrapper types
TEST(AttributeDataTypes, variantInTuple) {
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
TEST(AttributeDataTypes, vectorInTuple) {
  SaiAttributeTuple<B, VecAttr, I> t(B{true}, VecAttr{{42}}, I{420});
  std::tuple<bool, std::vector<uint64_t>, int32_t> vt = t.value();
  EXPECT_TRUE(std::get<0>(vt));
  EXPECT_EQ(std::get<1>(vt)[0], 42);
  EXPECT_EQ(std::get<2>(vt), 420);
}
TEST(AttributeDataTypes, optionalInTuple) {
  SaiAttributeTuple<B, AttrOptional, I> t(B{true}, AttrOptional{I{42}}, I{420});
  std::tuple<bool, std::optional<int32_t>, int32_t> vt = t.value();
  EXPECT_TRUE(std::get<0>(vt));
  EXPECT_EQ(std::get<1>(vt).value(), 42);
  EXPECT_EQ(std::get<2>(vt), 420);
}
TEST(AttributeDataTypes, tupleInTuple) {
  SaiAttributeTuple<B, SaiAttributeTuple<B, I>, I> t(
      B{true}, SaiAttributeTuple<B, I>{B{false}, I{42}}, I{420});
  std::tuple<bool, std::tuple<bool, int>, int> vt = t.value();
  EXPECT_TRUE(std::get<0>(vt));
  std::tuple<bool, int> nested = std::get<1>(vt);
  EXPECT_FALSE(std::get<0>(nested));
  EXPECT_EQ(std::get<1>(nested), 42);
  EXPECT_EQ(std::get<2>(vt), 420);
}
TEST(AttributeDataTypes, tupleInVariant) {
  SaiAttributeVariant<SaiAttributeTuple<B, I>, I> var(
      SaiAttributeTuple<B, I>{B{true}, I{42}});
  boost::variant<std::tuple<bool, int>, int> varv = var.value();
  std::tuple<bool, int> t = boost::get<std::tuple<bool, int>>(varv);
  EXPECT_TRUE(std::get<0>(t));
  EXPECT_EQ(std::get<1>(t), 42);
}
TEST(AttributeDataTypes, optionalInVariant) {
  SaiAttributeVariant<SaiAttributeOptional<I>, B> var(
      SaiAttributeOptional<I>{I{42}});
  boost::variant<std::optional<int>, bool> varv = var.value();
  std::optional<int> o = boost::get<std::optional<int>>(varv);
  EXPECT_EQ(o.value(), 42);
}
TEST(AttributeDataTypes, variantInVariant) {
  SaiAttributeVariant<SaiAttributeVariant<B, I>, I> var(
      SaiAttributeVariant<B, I>{I{42}});
  boost::variant<boost::variant<bool, int>, int> varv = var.value();
  boost::variant<bool, int> nested =
      boost::get<boost::variant<bool, int>>(varv);
  EXPECT_EQ(boost::get<int>(nested), 42);
}
TEST(AttributeDataTypes, tupleInOptional) {
  SaiAttributeOptional<SaiAttributeTuple<B, I>> o(
      SaiAttributeTuple<B, I>{B{true}, I{42}});
  std::optional<std::tuple<bool, int>> ov = o.value();
  EXPECT_EQ(std::get<0>(ov.value()), true);
  EXPECT_EQ(std::get<1>(ov.value()), 42);
}
TEST(AttributeDataTypes, variantInOptional) {
  SaiAttributeOptional<SaiAttributeVariant<B, I>> o(
      SaiAttributeVariant<B, I>{I{42}});
  std::optional<boost::variant<bool, int>> ov = o.value();
  EXPECT_EQ(boost::get<int>(ov.value()), 42);
}
TEST(AttributeDataTypes, optionalInOptional) {
  SaiAttributeOptional<SaiAttributeOptional<I>> o(
      SaiAttributeOptional<I>{I{42}});
  std::optional<std::optional<int>> ov = o.value();
  EXPECT_EQ(ov.value().value(), 42);
}

/*
 * Static assertion "tests" about SaiAttribute data types to ensure the types
 * behave how we expect them to.
 */

static_assert(
    std::is_same<
        typename AttrTuple::ValueType,
        std::tuple<bool, int32_t, std::vector<sai_object_id_t>>>::value,
    "SaiAttributeTuple::ValueType is not tuple of SaiAttribute ValueTypes");
static_assert(
    std::is_same<
        typename AttrVariant::ValueType,
        boost::variant<bool, int32_t, std::vector<sai_object_id_t>>>::value,
    "SaiAttributeVariant::ValueType is not tuple of SaiAttribute ValueTypes");
