// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_fatal_types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"
#include "fboss/thrift_cow/visitors/PathVisitor.h"

namespace facebook::fboss::thrift_cow {

using k = facebook::fboss::test_tags::strings;
using switch_config_k = facebook::fboss::cfg::switch_config_tags::strings;
using TestStructMembers = apache::thrift::reflect_struct<TestStruct>::member;
using TestUnionMembers =
    apache::thrift::reflect_variant<TestUnion>::traits::ids;
using L4PortRangeMembers =
    apache::thrift::reflect_struct<cfg::L4PortRange>::member;

TestStruct createSimpleTestStruct();

TestStruct createHybridMapTestStruct();

template <typename Node, typename Func>
inline ThriftTraverseResult visitPath(
    Node& node,
    pv_detail::PathIter begin,
    pv_detail::PathIter end,
    Func&& f) {
  auto op = pvlambda(std::forward<Func>(f));
  return PathVisitor<typename Node::TC>::visit(
      node, begin, end, PathVisitMode::LEAF, op);
}

} // namespace facebook::fboss::thrift_cow
