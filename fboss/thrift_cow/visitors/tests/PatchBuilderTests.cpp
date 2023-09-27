// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/visitors/PatchBuilder.h"
#include "fboss/thrift_cow/visitors/tests/VisitorTestUtils.h"

namespace facebook::fboss::thrift_cow {

TEST(PatchBuilderTests, simple) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;

  structB.inlineInt() = 123;

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(structB);

  auto patch = PatchBuilder::build(nodeA, nodeB, {});
  EXPECT_EQ(patch.get_basePath(), std::vector<std::string>());
  EXPECT_EQ(patch.patch()->getType(), PatchNode::Type::struct_node);
  auto children = patch.patch()->struct_node_ref()->children();
  // inlineInt id = 2
  auto inlineIntIt = children->find(2);
  EXPECT_TRUE(inlineIntIt != children->end());
  EXPECT_EQ(inlineIntIt->second.getType(), PatchNode::Type::val);
  auto value = deserializeBuf<apache::thrift::type_class::integral, int32_t>(
      fsdb::OperProtocol::COMPACT, std::move(*inlineIntIt->second.val_ref()));
  EXPECT_EQ(value, 123);
}
} // namespace facebook::fboss::thrift_cow
