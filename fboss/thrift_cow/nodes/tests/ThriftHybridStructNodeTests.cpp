/*
 *  Copyright (c) 2024-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include <thrift/lib/cpp2/reflection/testing.h>
#include "fboss/thrift_cow/nodes/Serializer.h"
#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"

#include <gtest/gtest.h>
#include <type_traits>

using namespace facebook::fboss;
using namespace facebook::fboss::thrift_cow;

using k = test_tags::strings;

template <typename ThriftType>
struct is_allow_skip_thrift_cow {
  using annotations = apache::thrift::reflect_struct<ThriftType>::annotations;
  static constexpr bool value =
      read_annotation_allow_skip_thrift_cow<annotations>::value;
};

TEST(ThriftHybridStructNodeTests, ReadThriftStructAnnotation) {
#ifdef __ENABLE_HYBRID_THRIFT_COW_TESTS__
  static_assert(is_allow_skip_thrift_cow<TestStruct>::value == true);
#endif // __ENABLE_HYBRID_THRIFT_COW_TESTS__
  static_assert(is_allow_skip_thrift_cow<ParentTestStruct>::value == false);
  static_assert(is_allow_skip_thrift_cow<TestStruct2>::value == false);
}

TEST(ThriftHybridStructNodeTests, ThriftStructNodeAnnotations) {
  ThriftStructFields<TestStruct> fields;

#ifdef __ENABLE_HYBRID_THRIFT_COW_TESTS__
  static_assert(fields.isSkipThriftCowEnabled<k::hybridMap>() == true);
#endif // __ENABLE_HYBRID_THRIFT_COW_TESTS__
  static_assert(fields.isSkipThriftCowEnabled<k::cowMap>() == false);

  ThriftStructNode<TestStruct> node;

#ifdef __ENABLE_HYBRID_THRIFT_COW_TESTS__
  ASSERT_EQ(node.isSkipThriftCowEnabled<k::hybridMap>(), true);
#endif // __ENABLE_HYBRID_THRIFT_COW_TESTS__
  ASSERT_EQ(node.isSkipThriftCowEnabled<k::cowMap>(), false);
}

TEST(ThriftHybridStructNodeTests, TestHybridNodeTypes) {
  ThriftStructNode<TestStruct> node;
  // cow member
  auto val = node.get<k::cowMap>();
  using underlying_type = folly::remove_cvref_t<decltype(*val)>::ThriftType;
  static_assert(std::is_same_v<underlying_type, std::map<int, bool>>);
  // hybrid member
#ifdef __ENABLE_HYBRID_THRIFT_COW_TESTS__
  auto valHybrid = node.get<k::hybridMap>();
  using underlying_type =
      folly::remove_cvref_t<decltype(*valHybrid)>::ThriftType;
  static_assert(std::is_same_v<underlying_type, std::map<int, bool>>);
  using ref_type = folly::remove_cvref_t<decltype(valHybrid->ref())>;
  static_assert(std::is_same_v<ref_type, std::map<int, bool>>);
#endif // __ENABLE_HYBRID_THRIFT_COW_TESTS__
}

TEST(ThriftHybridStructNodeTests, ThriftStructFieldsHybridMemberGetSet) {
  ThriftStructFields<TestStruct> fields;

  auto data = std::make_shared<std::map<int, bool>>();
  data->insert({1, false});
  data->insert({2, true});

  // cow member
  fields.set<k::cowMap>(*data);
  auto val = fields.get<k::cowMap>();
  ASSERT_EQ(val->size(), 2);
  ASSERT_EQ(val->at(1), false);
  ASSERT_EQ(val->at(2), true);
  // hybrid member
#ifdef __ENABLE_HYBRID_THRIFT_COW_TESTS__
  fields.set<k::hybridMap>(*data);
  auto valHybrid = fields.get<k::hybridMap>();
  ASSERT_EQ(valHybrid->cref().size(), 2);
  ASSERT_EQ(valHybrid->cref().at(1), false);
  ASSERT_EQ(valHybrid->cref().at(2), true);
#endif // __ENABLE_HYBRID_THRIFT_COW_TESTS__
}

#ifdef __ENABLE_HYBRID_THRIFT_COW_TESTS__
TEST(ThriftHybridStructNodeTests, ThriftStructFieldsHybridMemberToFromThrift) {
  ThriftStructFields<TestStruct> fields;

  auto data = std::make_shared<std::map<int, bool>>();
  data->insert({1, false});
  data->insert({2, true});

  // fromThrift
  auto val = fields.get<k::hybridMap>();
  val->fromThrift(*data);

  // toThrift
  auto thrift = val->toThrift();
  ASSERT_EQ(thrift, *data);
}

TEST(ThriftHybridStructNodeTests, ThriftStructNodeHybridMemberToFromThrift) {
  ThriftStructNode<TestStruct> node;

  auto data = std::make_shared<std::map<int, bool>>();
  data->insert({1, false});
  data->insert({2, true});

  // fromThrift
  auto val = node.get<k::hybridMap>();
  val->fromThrift(*data);

  // toThrift
  auto thrift = val->toThrift();
  ASSERT_EQ(thrift, *data);
}
#endif // __ENABLE_HYBRID_THRIFT_COW_TESTS__
