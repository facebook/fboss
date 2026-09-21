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

#include <optional>
#include <tuple>
#include <vector>

using namespace facebook::fboss;

template <typename T>
using A = SaiAttribute<sai_attr_id_t, 0, T>;
using VecAttr = SaiAttribute<sai_attr_id_t, 0, std::vector<sai_object_id_t>>;
using B = A<bool>;
using I = A<int32_t>;
using AttrTuple = std::tuple<B, I, VecAttr>;
using AttrOptional = std::optional<I>;

TEST(AttributeDataTypes, attributeTupleGetSaiAttr) {
  std::vector<sai_object_id_t> vec;
  vec.resize(42);
  AttrTuple at{B{true}, I{42}, VecAttr{vec}};
  std::vector<sai_attribute_t> attrV = saiAttrs(at);
  EXPECT_EQ(attrV.size(), 3);
  EXPECT_TRUE(attrV[0].value.booldata);
  EXPECT_EQ(attrV[1].value.s32, 42);
  EXPECT_EQ(attrV[2].value.objlist.count, 42);
}

TEST(AttributeDataTypes, attributeOptionalGetSaiAttr) {
  AttrOptional o(I{42});
  const sai_attribute_t* attr = saiAttr(o);
  EXPECT_TRUE(attr);
  EXPECT_EQ(attr->value.s32, 42);
}

TEST(AttributeDataTypes, attributeOptionalGetValueNone) {
  AttrOptional o;
  const sai_attribute_t* attr = saiAttr(o);
  EXPECT_FALSE(attr);
}
