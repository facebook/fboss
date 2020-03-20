/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/LoggingUtil.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"

#include <folly/logging/xlog.h>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

template <typename T>
using A = SaiAttribute<sai_attr_id_t, 0, T>;
using ObjAttr = SaiAttribute<sai_attr_id_t, 0, SaiObjectIdT>;
using UInt64TAttr = SaiAttribute<sai_attr_id_t, 0, sai_uint64_t>;
using VecAttr = SaiAttribute<sai_attr_id_t, 0, std::vector<sai_object_id_t>>;
using MacAttr = SaiAttribute<sai_attr_id_t, 0, folly::MacAddress>;

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

// Test that it is possible to use an attribute as a hash table key
TEST(Attribute, hashKey) {
  std::unordered_map<ObjAttr, int> objMap;
  ObjAttr o1{42};
  ObjAttr o2{1024};
  objMap[o1] = 42;
  objMap[o2] = 1024;
  for (const auto& obj : objMap) {
    EXPECT_EQ(obj.first.value(), obj.second);
  }
}

// Test formatting attributes
template <>
struct facebook::fboss::AttributeName<ObjAttr> {
  static const inline std::string value = "ObjAttr";
};
TEST(Attribute, formatOid) {
  ObjAttr o{42};
  std::string expected("ObjAttr: 42");
  EXPECT_EQ(expected, fmt::format("{}", o));
}

template <>
struct facebook::fboss::AttributeName<MacAttr> {
  static const inline std::string value = "MacAttr";
};
TEST(Attribute, formatMac) {
  std::string macStr("de:ad:be:ef:42:42");
  MacAttr a{folly::MacAddress{macStr}};
  std::string expected("MacAttr: de:ad:be:ef:42:42");
  EXPECT_EQ(expected, fmt::format("{}", a));
}

TEST(Attribute, formatAttrTuple) {
  ObjAttr o{42};
  std::string macStr("de:ad:be:ef:42:42");
  MacAttr a{folly::MacAddress{macStr}};
  std::tuple<ObjAttr, MacAttr> tup{o, a};
  std::string expected("(ObjAttr: 42, MacAttr: de:ad:be:ef:42:42)");
  EXPECT_EQ(expected, fmt::format("{}", tup));
}

/*
 * Static assertion "tests" about SaiAttributes to ensure the types behave
 * how we expect them to.
 */

// SaiObjectIdT
static_assert(
    !IsSaiTypeWrapper<SaiObjectIdT>::value,
    "SaiObjectIdT is not a value wrapper");
static_assert(
    IsDuplicateSaiType<SaiObjectIdT>::value,
    "SaiObjectIdT is a helper for duplicate sai type in union");
static_assert(
    std::is_same<
        typename DuplicateTypeFixer<SaiObjectIdT>::value,
        sai_object_id_t>::value,
    "SaiObjectIdT resolves duplication for sai_object_id_t");

// folly::IPAddress
static_assert(
    IsSaiTypeWrapper<folly::IPAddress>::value,
    "folly::IPAddress is a wrapper for a SAI type");
static_assert(
    std::is_same<
        typename WrappedSaiType<folly::IPAddress>::value,
        sai_ip_address_t>::value,
    "folly::IPAddress wraps sai_ip_address_t");

// std::vector<sai_object_id_t>
static_assert(
    IsSaiTypeWrapper<std::vector<sai_object_id_t>>::value,
    "std::vector<sai_object_id_t> is a wrapper for a SAI type");
static_assert(
    std::is_same<
        typename WrappedSaiType<std::vector<sai_object_id_t>>::value,
        sai_object_list_t>::value,
    "std::vector<sai_object_id_t> wraps sai_object_list_t");

// int32_t
static_assert(
    !IsDuplicateSaiType<int32_t>::value,
    "int32_t is not a helper for duplicate sai type in union");
static_assert(
    !IsSaiTypeWrapper<int32_t>::value,
    "int32_t is not a value wrapper");

// IsSaiAttribute
// basic data type
static_assert(
    IsSaiAttribute<ObjAttr>::value,
    "ObjAttr should be a SAI Attribute");

// complex data type
static_assert(
    IsSaiAttribute<VecAttr>::value,
    "VecAttr should be a SAI Attribute");
