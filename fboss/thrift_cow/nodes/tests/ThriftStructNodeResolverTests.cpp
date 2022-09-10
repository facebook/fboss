// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"

#include <gtest/gtest.h>

namespace facebook::fboss::thrift_cow {

using k = test_tags::strings;

template <typename TType>
struct TestResolver : public DefaultTypeResolver<TType> {};

class DerivedTestStructNode;
template <>
struct TestResolver<TestStruct> {
  using type = DerivedTestStructNode;
};

class DerivedTestStructNode
    : public ThriftStructNode<TestStruct, TestResolver> {
 public:
  using Base = ThriftStructNode<TestStruct, TestResolver>;
  using Base::Base;

  int getInlineInt() const {
    return get<k::inlineInt>()->ref();
  }

  void setInlineInt(int i) {
    set<k::inlineInt>(i);
  }

 private:
  friend class CloneAllocator;
};

TEST(ThriftStructNodeResolverTests, DerivedTestStructNodeTest) {
  DerivedTestStructNode node;
  node.setInlineInt(123);
  EXPECT_EQ(node.getInlineInt(), 123);
}

} // namespace facebook::fboss::thrift_cow
