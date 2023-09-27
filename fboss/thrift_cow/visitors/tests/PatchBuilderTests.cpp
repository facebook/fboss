// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/visitors/PatchBuilder.h"
#include "fboss/thrift_cow/visitors/tests/VisitorTestUtils.h"

namespace facebook::fboss::thrift_cow {

TEST(PatchBuilderTests, empty) {
  auto structA = createSimpleTestStruct();
  auto structB = structA;

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(structA);
  auto nodeB = std::make_shared<ThriftStructNode<TestStruct>>(structB);

  PatchBuilder::build(nodeA, nodeB, {});
}
} // namespace facebook::fboss::thrift_cow
