// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/gen-cpp2/switch_config_fatal_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_types.h"
#include "fboss/thrift_cow/visitors/PatchApplier.h"
#include "fboss/thrift_cow/visitors/PatchBuilder.h"
#include "fboss/thrift_cow/visitors/tests/VisitorTestUtils.h"

#include <gtest/gtest.h>
#include <memory>

namespace {
using namespace facebook::fboss;
using namespace facebook::fboss::thrift_cow;

void testPatchBuildApply(
    std::shared_ptr<ThriftStructNode<TestStruct>> nodeA,
    std::shared_ptr<ThriftStructNode<TestStruct>> nodeB) {
  auto patch = *PatchBuilder::build(
                    nodeA,
                    nodeB,
                    {},
                    fsdb::OperProtocol::COMPACT,
                    true /* incrementallyCompress */)
                    .patch();
  auto nodeC = nodeA->clone();
  auto ret = RootPatchApplier::apply(*nodeC, std::move(patch));
  EXPECT_EQ(ret, PatchApplyResult::OK);
  EXPECT_EQ(nodeB->toThrift(), nodeC->toThrift());
}

} // namespace

namespace facebook::fboss::thrift_cow::test {

using k = facebook::fboss::test_tags::strings;
using config_k = facebook::fboss::cfg::switch_config_tags::strings;

TEST(PatchBuildApplyTests, TestTruncationPrimitives) {
  auto s = createSimpleTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(s);
  nodeA->publish();
  auto nodeB = nodeA->clone();

  nodeB->ref<k::inlineInt>() = *s.inlineInt() + 1;
  nodeB->ref<k::inlineString>() = "some new string";

  testPatchBuildApply(nodeA, nodeB);
}

TEST(PatchBuildApplyTests, TestTruncationInlineStruct) {
  auto s = createSimpleTestStruct();
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(s);
  nodeA->publish();
  auto nodeB = nodeA->clone();

  auto inlineStruct = nodeB->modify<k::inlineStruct>();
  inlineStruct->ref<config_k::min>() = 1234567;
  inlineStruct->ref<config_k::max>() = 987654;

  testPatchBuildApply(nodeA, nodeB);
}

TEST(PatchBuildApplyTests, TestTruncationMap) {
  auto s = createSimpleTestStruct();
  s.mapOfI32ToI32()[1] = 123;
  s.mapOfI32ToI32()[2] = 456;
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(s);
  nodeA->publish();
  auto nodeB = nodeA->clone();

  auto map = nodeB->modify<k::mapOfI32ToI32>();
  // modify add and remove
  map->ref(1) = 1;
  map->emplace(312, 9);
  map->remove(2);

  testPatchBuildApply(nodeA, nodeB);
}

TEST(PatchBuildApplyTests, TestTruncationList) {
  auto s = createSimpleTestStruct();
  s.listOfPrimitives() = {2, 3};
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(s);
  nodeA->publish();
  auto nodeB = nodeA->clone();

  auto list = nodeB->modify<k::listOfPrimitives>();
  // modify and add
  list->ref(1) = 1;
  list->emplace_back(4);

  testPatchBuildApply(nodeA, nodeB);
}

TEST(PatchBuildApplyTests, TestTruncationSet) {
  auto s = createSimpleTestStruct();
  s.setOfI32() = {2, 3};
  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(s);
  nodeA->publish();
  auto nodeB = nodeA->clone();

  auto set = nodeB->modify<k::setOfI32>();
  // add and remove
  set->emplace(1);
  set->remove(3);

  testPatchBuildApply(nodeA, nodeB);
}

TEST(PatchBuildApplyTests, TestTruncactionNestedStruct) {
  auto s = createSimpleTestStruct();
  s.mapOfI32ToListOfStructs()[1].emplace_back();
  (*s.mapOfI32ToListOfStructs())[1][0].min() = 1;

  auto nodeA = std::make_shared<ThriftStructNode<TestStruct>>(s);
  nodeA->publish();
  auto nodeB = nodeA->clone();

  auto map = nodeB->modify<k::mapOfI32ToListOfStructs>();
  map->modifyTyped(1);
  auto list = map->at(1);
  list->modify(0);

  // modify and add
  nodeB->ref<k::mapOfI32ToListOfStructs>()->at(1)->at(0)->ref<config_k::min>() =
      999;
  nodeB->ref<k::mapOfI32ToListOfStructs>()->at(1)->emplace_back();
  nodeB->ref<k::mapOfI32ToListOfStructs>()->at(1)->at(1)->ref<config_k::max>() =
      777;

  testPatchBuildApply(nodeA, nodeB);
}

} // namespace facebook::fboss::thrift_cow::test
