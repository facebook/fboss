// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"

#include <gtest/gtest.h>

namespace facebook::fboss::thrift_cow {

using k = test_tags::strings;

ADD_THRIFT_RESOLVER_MAPPING(TestStruct, DerivedTestStructNode);
class DerivedTestStructNode : public ThriftStructNode<TestStruct> {
 public:
  using Base = ThriftStructNode<TestStruct>;
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

ADD_THRIFT_RESOLVER_MAPPING(ParentTestStruct, DerivedParentTestNode);
class DerivedParentTestNode : public ThriftStructNode<ParentTestStruct> {
 public:
  using Base = ThriftStructNode<ParentTestStruct>;
  using Base::Base;

  std::shared_ptr<DerivedTestStructNode> getChild() const {
    return get<k::childStruct>();
  }

  void setChild(std::shared_ptr<DerivedTestStructNode> node) {
    ref<k::childStruct>() = node;
  }

 private:
  friend class CloneAllocator;
};

TEST(ThriftStructNodeResolverTests, DerivedParentTestStructNodeTest) {
  DerivedParentTestNode node;
  auto child = std::make_shared<DerivedTestStructNode>();
  child->setInlineInt(321);
  node.setChild(child);
  EXPECT_EQ(node.getChild()->getInlineInt(), 321);
}

} // namespace facebook::fboss::thrift_cow
