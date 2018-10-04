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

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

template <typename T>
using A = SaiAttribute<sai_attr_id_t, 0, T>;

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

using MacAttr = SaiAttribute<sai_attr_id_t, 0, sai_mac_t, folly::MacAddress>;
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

using ListAttr = SaiAttribute<
    sai_attr_id_t,
    0,
    sai_object_list_t,
    std::vector<sai_object_id_t>>;
TEST(Attribute, getList) {
  std::vector<sai_object_id_t> v;
  v.resize(4);
  ListAttr a(v);
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

TEST(Attribute, ctorList) {
  std::vector<sai_object_id_t> v{0, 1, 4, 9};
  ListAttr a(v);
  for (int i = 0; i < a.saiAttr()->value.objlist.count; ++i) {
    EXPECT_EQ(a.saiAttr()->value.objlist.list[i], i * i);
  }
}

TEST(Attribute, lifetimeList) {
  std::vector<sai_object_id_t> v2;
  {
    std::vector<sai_object_id_t> v{0, 1, 4, 9};
    ListAttr a(v);
    v2 = a.value();
  }
  std::vector<sai_object_id_t> v{0, 1, 4, 9};
  EXPECT_EQ(v, v2);
}
